import type { Assignment, Submission, GradeResponse } from './types';


export const demoAssignments: Assignment[] = [
    { id: 'a1', title: 'Basic SELECT & WHERE', course: 'DB101 — Intro to SQL', difficulty: 'Beginner', points: 10, due: '2025-09-05', tags: ['SELECT', 'WHERE', 'FILTERS'], attemptsAllowed: 3 },
    { id: 'a2', title: 'JOINs & Aggregates', course: 'DB201 — Intermediate SQL', difficulty: 'Intermediate', points: 20, due: '2025-09-12', tags: ['JOIN', 'GROUP BY', 'HAVING'], attemptsAllowed: 3 },
];


export const demoSubmissions: Submission[] = [
    { id: 's1', student: 'A. Janssen', assignment: 'Basic SELECT & WHERE', date: '2025-08-28 16:02', grade: 8.5, status: 'Auto-graded' },
    { id: 's2', student: 'M. de Vries', assignment: 'JOINs & Aggregates', date: '2025-08-27 10:41', grade: 6.0, status: 'Needs review' },
];


export function simulateCppGrade(sql: string): Promise<GradeResponse> {
    const normalized = (sql || '').toLowerCase();
    const correctness = normalized.includes('select') ? 0.7 : 0.2;
    const style = normalized.includes('join') ? 0.8 : 0.6;
    const efficiency = normalized.includes('where') ? 0.75 : 0.5;
    const grade = Math.round(((correctness + style + efficiency) / 3) * 10 * 10) / 10;
    const feedback = [
        correctness > 0.6 ? 'Query returns the expected columns for most test cases.' : 'Result shape mismatches the expected output in several tests.',
        style > 0.7 ? 'Joins are used appropriately and aliases improve readability.' : 'Consider clearer aliasing and consistent indentation.',
        efficiency > 0.7 ? 'WHERE filters are effective; indexes likely reduce scan cost.' : 'Consider filtering earlier or limiting the result set.',
    ];
    const rubric = { correctness: Math.round(correctness * 100), style: Math.round(style * 100), efficiency: Math.round(efficiency * 100) };
    return new Promise((resolve) => setTimeout(() => resolve({ grade, feedback, rubric }), 600));
}