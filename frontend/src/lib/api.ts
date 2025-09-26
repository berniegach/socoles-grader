import Papa from 'papaparse';
import type { GradingOptions } from '@/lib/types';

// If running server-side, prefer internal container network endpoint, else use public URL for browser
const isServer = typeof window === 'undefined';
export const API_BASE = (isServer ? (process.env.SOCOLES_INTERNAL_API_URL || process.env.NEXT_PUBLIC_SOCOLES_API_URL) : process.env.NEXT_PUBLIC_SOCOLES_API_URL) || 'http://localhost:5000';
export const GRADE_PATH = process.env.NEXT_PUBLIC_SOCOLES_GRADE_PATH || '/grade-queries';
export const FORM_KEYS = {
    students: process.env.NEXT_PUBLIC_SOCOLES_STUDENTS_KEY || 'students',
    references: process.env.NEXT_PUBLIC_SOCOLES_REFERENCES_KEY || 'references',
    initSql: process.env.NEXT_PUBLIC_SOCOLES_INITSQL_KEY || 'init_sql',
} as const;

// NOTE: In interactive grading flows we now use per-question initSql only.
// The batch grader here still accepts a single init SQL file representing the environment
// to apply for all graded queries in that batch.

// Default grading options 
export const DEFAULT_GRADING_OPTIONS: GradingOptions = {
    syntaxSensitivity: '3 Levels',
    semanticsSensitivity: '8 Levels',
    resultsSensitivity: '3 Levels',
    evaluationPriority: '5 - Semantics, Results, Syntax',
    textEditDistance: '4',
    treeEditDistance: '4',
    checkOrder: false,
    autoDB: false,
    numberOfDBs: '',
    dbName: '',
    use_postgresql: true,
};

function sanitizeOptions(input?: Partial<GradingOptions>): GradingOptions {
    const merged: GradingOptions = { ...DEFAULT_GRADING_OPTIONS, ...(input || {}) } as GradingOptions;

    // Replace empty selections with defaults
    const selKeys: Array<keyof GradingOptions> = ['syntaxSensitivity', 'semanticsSensitivity', 'resultsSensitivity', 'evaluationPriority'];
    for (const k of selKeys) {
        const v = merged[k] as unknown as string;
        if (typeof v !== 'string' || v.trim() === '') (merged as any)[k] = (DEFAULT_GRADING_OPTIONS as any)[k];
    }

    // Coerce numeric string thresholds to non-negative numbers, fallback to defaults
    const numStr = (s: string, min: number, dflt: string) => {
        const n = Number(s);
        if (!isFinite(n) || isNaN(n) || n < min) return dflt;
        return String(n);
    };
    merged.textEditDistance = numStr(merged.textEditDistance, 0, DEFAULT_GRADING_OPTIONS.textEditDistance);
    merged.treeEditDistance = numStr(merged.treeEditDistance, 0, DEFAULT_GRADING_OPTIONS.treeEditDistance);

    // Auto DB handling
    if (!merged.autoDB) {
        merged.dbName = '';
        // keep numberOfDBs sane but it will be ignored by backend if autoDB=false
        merged.numberOfDBs = numStr(merged.numberOfDBs, 1, DEFAULT_GRADING_OPTIONS.numberOfDBs);
    } else {
        merged.dbName = (merged.dbName || '').trim();
        merged.numberOfDBs = numStr(merged.numberOfDBs, 1, DEFAULT_GRADING_OPTIONS.numberOfDBs);
    }

    // Booleans already default via spread
    merged.checkOrder = !!merged.checkOrder;
    merged.use_postgresql = !!merged.use_postgresql;

    return merged;
}

// Helper to parse model queries (single-column CSV, may include quotes)
function parseModelQueries(text: string): string[] {
    if (!text) return [];
    const parsed = Papa.parse(text, { header: false, skipEmptyLines: true });
    const rows: unknown[] = Array.isArray(parsed.data) ? (parsed.data as unknown[]) : [];
    return rows
        .map((r) => (Array.isArray(r) ? String((r as unknown[])[0] ?? '').trim() : String(r ?? '').trim()))
        .filter((s) => s.length > 0);
}

// Helper to parse student queries with headers
function parseStudentQueries(text: string): Array<[string, number, string, string]> {
    if (!text) return [];
    const parsed = Papa.parse(text, { header: true, skipEmptyLines: true });
    const data: Record<string, string>[] = Array.isArray(parsed.data) ? (parsed.data as Record<string, string>[]) : [];
    const pick = (row: Record<string, string>, key: string) => (row[key] ?? '').toString().trim();
    return data
        .filter((row) => pick(row, 'Org Defined ID') && pick(row, 'Q #') && pick(row, 'Answer'))
        .map((row) => [
            pick(row, 'Org Defined ID'),
            parseInt(pick(row, 'Attempt #') || '0', 10) || 0,
            pick(row, 'Q #'),
            pick(row, 'Answer'),
        ]);
}

export async function sendBatchToSocoles({ studentsFile, referencesFile, initSqlFile, options, selectedQuestionNumber }: { studentsFile?: File; referencesFile?: File; initSqlFile?: File; options?: Partial<GradingOptions>; selectedQuestionNumber?: string; }) {
    // Read files as text
    const studentCsvText = studentsFile ? await studentsFile.text() : '';
    const refCsvText = referencesFile ? await referencesFile.text() : '';
    const sqlText = initSqlFile ? await initSqlFile.text() : '';

    // Parse CSVs
    let studentQueries = parseStudentQueries(studentCsvText);
    const modelQueries = parseModelQueries(refCsvText);

    // Filter by selected question number if provided
    if (selectedQuestionNumber && selectedQuestionNumber.trim().length > 0) {
        studentQueries = studentQueries.filter((row) => row[2].toString().trim() === selectedQuestionNumber.trim());
    }

    // Merge and map grading options
    const mergedOptions = sanitizeOptions(options);
    const payload = {
        sql_data: sqlText, // Contents of the init SQL file (create+insert)
        queries: studentQueries, // [Org Defined ID, Attempt #, Q #, Answer]
        model_queries: modelQueries, // array of strings
        syntax: parseInt(mergedOptions.syntaxSensitivity, 10) || 0,
        semantics: parseInt(mergedOptions.semanticsSensitivity, 10) || 0,
        results: parseInt(mergedOptions.resultsSensitivity, 10) || 0,
        prop_order: parseInt(mergedOptions.evaluationPriority, 10) || 0,
        edit_dist: parseInt(mergedOptions.textEditDistance, 10) || 0,
        tree_dist: parseInt(mergedOptions.treeEditDistance, 10) || 0,
        check_order: mergedOptions.checkOrder ? 1 : 0,
        auto_db: mergedOptions.autoDB ? 1 : 0,
        num_db: parseInt(mergedOptions.numberOfDBs, 10) || 0,
        sql_create_data: sqlText, // Use same SQL for create if only one file is provided
        dbname: mergedOptions.dbName || '',
        use_postgresql: !!mergedOptions.use_postgresql,
    };

    const resp = await fetch(`${API_BASE}${GRADE_PATH}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
    });

    const ctype = resp.headers.get('content-type') || '';
    if (!resp.ok) {
        const text = await resp.text().catch(() => '');
        throw new Error(`Backend error (${resp.status}): ${text || 'request failed'}`);
    }
    if (ctype.includes('application/json')) return { type: 'json', payload: await resp.json() as unknown } as const;

    const blob = await resp.blob();
    const text = await blob.text();
    const parsed = Papa.parse(text, { header: true, skipEmptyLines: true });
    return { type: 'csv', payload: parsed.data as Record<string, unknown>[], raw: text } as const;
}


export async function parseCsvPreview(file: File) {
    return new Promise<{ headers: string[]; rows: Record<string, unknown>[] }>((resolve, reject) => {
        Papa.parse(file, {
            header: true,
            skipEmptyLines: true,
            complete: (res: any) => resolve({ headers: (res.meta?.fields as string[]) || [], rows: (res.data as Record<string, unknown>[]) || [] }),
            error: (err: unknown) => reject(err),
        });
    });
}


export async function parseCsvNoHeader(file: File) {
    return new Promise<{ headers: string[]; rows: Array<{ statement: string }> }>((resolve, reject) => {
        Papa.parse(file, {
            header: false,
            skipEmptyLines: true,
            complete: (res: any) => {
                const rows = ((res.data as unknown[]) || []).map((r: unknown) => ({ statement: Array.isArray(r) ? String((r as unknown[])[0] ?? '') : String(r ?? '') }));
                resolve({ headers: ['statement'], rows });
            },
            error: (err: unknown) => reject(err),
        });
    });
}