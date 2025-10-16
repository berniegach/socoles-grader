import type { Assignment, Submission, GradeResponse } from './types';


export const demoAssignments: Assignment[] = [
    { id: 'a1', title: 'Basic SELECT & WHERE', course: 'DB101 — Intro to SQL', difficulty: 'Beginner', points: 10, due: '2025-09-05', tags: ['SELECT', 'WHERE', 'FILTERS'], attemptsAllowed: 3, published: true },
    { id: 'a2', title: 'JOINs & Aggregates', course: 'DB201 — Intermediate SQL', difficulty: 'Intermediate', points: 20, due: '2025-09-12', tags: ['JOIN', 'GROUP BY', 'HAVING'], attemptsAllowed: 3, published: true },
];


export const demoSubmissions: Submission[] = [
    { id: 's1', student: 'A. Janssen', assignmentId: 'a1', date: '2025-08-28 16:02', grade: 8.5, status: 'Auto-graded' },
    { id: 's2', student: 'M. de Vries', assignmentId: 'a2', date: '2025-08-27 10:41', grade: 6.0, status: 'Needs review' },
];


export function simulateCppGrade(sql: string): Promise<GradeResponse> {
    const normalized = (sql || '').toLowerCase();
    const syntax = normalized.includes('select') ? 0.7 : 0.2;
    const semantics = normalized.includes('join') ? 0.8 : 0.6;
    const results = normalized.includes('where') ? 0.75 : 0.5;
    const grade = Math.round(((syntax + semantics + results) / 3) * 10 * 10) / 10;
    const feedback = [
        syntax > 0.6 ? 'Basic SQL structure is mostly correct.' : 'Syntax issues detected in clauses or order.',
        semantics > 0.7 ? 'Joins and relationships inferred correctly.' : 'Potential logical issues in joins or aggregations.',
        results > 0.7 ? 'Output tuples align with expected test results.' : 'Returned rows/values differ from expected outcomes.',
    ];
    const rubric = { syntax, semantics, results };
    return new Promise((resolve) => setTimeout(() => resolve({ grade, feedback, rubric }), 600));
}