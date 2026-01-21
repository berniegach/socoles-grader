export type Role = 'student' | 'instructor';
export interface Rubric { syntax: number | null; semantics: number | null; results: number | null; absent?: { syntax?: boolean; semantics?: boolean; results?: boolean } }
export interface GradeResponse { grade: number; feedback: string[]; rubric: Rubric }
// Multi-tenant additions: instructor ownership & course instances
export interface Instructor { id: string; email: string; name: string; createdAt: string }
export interface CourseInstance { id: string; ownerId: string; title: string; code: string; term?: string; createdAt: string }

export interface Assignment { id: string; title: string; course: string; difficulty: string; points: number; due: string; tags: string[]; attemptsAllowed: number; published: boolean; ownerId?: string; courseInstanceId?: string }
export interface Question { id: string; title: string; difficulty: string; status: string; attempts: number; maxPoints: number; dataset?: string; prompt?: string; modelSql?: string; hints?: string; modelQueries?: string[]; initSql?: string; useDefaultGrading?: boolean; gradingOptions?: GradingOptions; ownerId?: string }
export interface Submission { id: string; student: string; assignmentId: string; date: string; grade: number; status: string }
// Lightweight user info used in API responses (avoid exposing token/types)
export interface UserLite { id: string; name: string; email: string; role: Role }
// Dataset entity
export interface Dataset { id: string; name: string; sql: string; createdAt?: string; updatedAt?: string }
// Assignment PATCH payload convenience
export interface AssignmentPatch { id: string; title?: string; points?: number; due?: string; difficulty?: string; tags?: string[]; attemptsAllowed?: number; published?: boolean }
// Rich question detail (adds optional loaded fields)
export interface QuestionDetail extends Question { createdAt?: string; updatedAt?: string; publishedAt?: string }
// Submission record with related questions (flattened)
export interface SubmissionRecord extends Submission { questions?: QuestionSubmission[] }
// Draft state for question submission editing in front-end
export interface QuestionSubmissionDraft { sql: string; grade: number | null; feedback: string[]; rubric: QuestionRubric | null }
// Attempt history entry (mirrors QuestionSubmissionAttempt minimal view)
export interface AttemptHistory { id: string; attempt: number; sql: string; grade: number | null; feedback: string[]; createdAt?: string }

// Join link + enriched assignment types
export interface AssignmentQuestionLink { assignmentId: string; questionId: string; position: number; pointsOverride?: number | null }
export interface AssignmentWithQuestions extends Assignment {
    questions: Array<{
        id: string; title: string; difficulty: string; status: string; attempts: number; maxPoints: number; position: number; pointsOverride?: number | null;
    }>
}

export interface NewAssignmentPayload { title: string; difficulty: string; points: number; due: string; tags: string[]; attemptsAllowed: number; published?: boolean; course?: string }
export interface NewSubmissionPayload { student: string; assignment: string; date?: string; grade?: number; status?: string }

// Options used by the instructor BatchGrader before running grading
export interface GradingOptions {
    syntaxSensitivity: string;
    semanticsSensitivity: string;
    resultsSensitivity: string;
    evaluationPriority: string;
    textEditDistance: string; // string for easy TextField binding
    treeEditDistance: string; // string for easy TextField binding
    checkOrder: boolean;
    autoDB: boolean;
    numberOfDBs: string; // keep as string for easy TextField binding
    dbName: string;
    use_postgresql: boolean;
}

// DB entities extra (optional fields for creation form)
export interface NewQuestionPayload {
    title: string;
    difficulty: string;
    maxPoints: number;
    dataset: string;
    prompt: string;
    modelSql: string;
    hints: string;
    publish?: boolean;
    modelQueries?: string[];
    initSql?: string;
    useDefaultGrading?: boolean;
    gradingOptions?: GradingOptions;
}

// Rubric structure used for per-question grading persistence
export interface QuestionRubric {
    syntax: number | null;
    semantics: number | null;
    results: number | null;
    absent?: { syntax?: boolean; semantics?: boolean; results?: boolean };
    [extra: string]: number | null | object | undefined;
}

export interface QuestionSubmission {
    id: string;
    submissionId: string;
    assignmentId: string;
    questionId: string;
    student: string;
    sql: string;
    grade: number | null;
    status: string;
    rubric: QuestionRubric | null;
    feedback: string[];
    title?: string;
    attempt?: number;
}
export interface SubmissionWithQuestions extends Submission { questions?: QuestionSubmission[] }

export interface QuestionSubmissionAttempt {
    id: string;
    questionSubmissionId: string | null;
    submissionId: string;
    assignmentId: string;
    questionId: string;
    student: string;
    sql: string;
    grade: number | null;
    status: string;
    rubric: QuestionRubric | null;
    feedback: string[];
    attempt: number;
    manual?: boolean;
    createdAt: string;
}

export interface ReviewRequest {
    id: string; assignmentId: string; questionId: string; submissionId: string; student: string; comment: string; status: string; createdAt: string; updatedAt: string; instructorReply?: string | null; replyAt?: string | null;
}

export interface ReviewRequestMessage {
    id: string;
    requestId: string;
    senderRole: 'student' | 'instructor';
    sender: string;
    message: string;
    createdAt: string;
}

// Grading workflow DTOs
export interface GradeSubmissionRequest {
    submissionId: string;
    noAttemptIncrement?: boolean;
}
export interface PerQuestionGradeResult {
    questionId: string;
    id?: string; // question_submission id
    title: string;
    grade: number | null;
    status: string;
    error?: string;
}
export interface GradeSubmissionResponse {
    submission: Submission;
    results: PerQuestionGradeResult[];
    status: string; // overall status
}

// Class roster
export type RosterStatus = 'Pending' | 'Invited' | 'Active' | 'Dropped' | 'Removed';
export interface RosterEntry { id: string; name: string; email: string; status: RosterStatus; evaluator?: boolean }
export interface NewRosterEntry { name: string; email: string; status?: RosterStatus }

// Invites
export interface InviteRecord { id: string; rosterId: string; email: string; name: string; token: string; expiresAt?: string | null; usedAt?: string | null }
export interface InviteResponse extends InviteRecord { link: string }