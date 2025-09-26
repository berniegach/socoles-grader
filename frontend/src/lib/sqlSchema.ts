export type TableSchema = { name: string; columns: Array<{ name: string; pk: boolean }> };

// Extract schema from init SQL (table, columns, primary key)
export function extractSchema(sql: string): TableSchema[] {
    try {
        const cleaned = String(sql || '')
            .replace(/--.*$/gm, '')
            .replace(/\/\*[\s\S]*?\*\//g, '');
        const out: TableSchema[] = [];
        const re = /create\s+table\s+(["`\[]?[\w.]+["`\]]?)\s*\(([^;]*)\)/gi;
        let m: RegExpExecArray | null;
        while ((m = re.exec(cleaned)) !== null) {
            const rawName = (m[1] || '').trim();
            const name = rawName.replace(/["`\[\]]/g, '');
            const body = (m[2] || '').trim();
            const parts = body.split(/,(?![^()]*\))/).map(s => s.trim()).filter(Boolean);
            const pkSet = new Set<string>();
            for (const line of parts) {
                const l = line.replace(/\s+/g, ' ').trim();
                const pkMatch = l.match(/^(?:constraint\s+\S+\s+)?primary\s+key\s*\(([^)]+)\)/i);
                if (pkMatch) {
                    pkMatch[1]
                        .split(',')
                        .map(s => s.trim().replace(/["`\[\]]/g, '').toLowerCase())
                        .forEach(c => pkSet.add(c));
                }
            }
            const colDefs: Array<{ name: string; pk: boolean }> = [];
            for (const line of parts) {
                const l = line.replace(/\s+/g, ' ').trim();
                if (/^(constraint|primary|foreign|unique|check|key|references)\b/i.test(l)) continue;
                const colNameRaw = l.split(' ')[0] || '';
                const colName = colNameRaw.replace(/["`\[\]]/g, '');
                const isInlinePk = /\bprimary\s+key\b/i.test(l);
                const isPk = isInlinePk || pkSet.has(colName.toLowerCase());
                colDefs.push({ name: colName, pk: !!isPk });
            }
            out.push({ name, columns: colDefs });
        }
        return out;
    } catch {
        return [];
    }
}
