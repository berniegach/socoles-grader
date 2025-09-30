export interface DashboardMetrics {
    pendingReviews: number;
    avgGrade: number; // 0..1 normalized
    passRate: number; // 0..1
    avgAttempts: number; // mean attempts per question (if available)
    gradeTrend: Array<{ date: string; avg: number }>;
    submissionsTrend: Array<{ date: string; count: number }>;
    questionDifficulty: Array<{ id: string; title: string; assignmentId?: string; assignmentTitle?: string; attempts: number; best: number }>;
    atRisk: Array<{ id: string; title: string; assignmentId?: string; assignmentTitle?: string; attempts: number; best: number }>;
    recent: Array<{ id: string; student: string; assignment: string; date: string; grade: number | null; status: string }>;
}

export const emptyMetrics: DashboardMetrics = {
    pendingReviews: 0,
    avgGrade: 0,
    passRate: 0,
    avgAttempts: 0,
    gradeTrend: [],
    submissionsTrend: [],
    questionDifficulty: [],
    atRisk: [],
    recent: [],
};
