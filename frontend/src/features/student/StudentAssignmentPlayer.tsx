"use client";
import { useEffect, useState, useCallback, useMemo, useRef } from 'react';
import { Box, Card, CardHeader, CardContent, CardActions, Typography, Button, Chip, Tabs, Tab, Divider, Collapse, CircularProgress } from '@mui/material';
import { formatDateTimeDDMMYYYYHHmm } from '@/lib/format';
import DifficultyChip from '@/components/DifficultyChip';
import SqlEditor from '@/components/SqlEditor';
import PromptWithHint from '@/components/PromptWithHint';
import SchemaPreview from '@/components/SchemaPreview';
import RubricBars from '@/components/RubricBars';
import FeedbackList from '@/components/FeedbackList';
import GradeDisplay from '@/components/GradeDisplay';
import PlayArrowIcon from '@mui/icons-material/PlayArrow';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ArrowForwardIcon from '@mui/icons-material/ArrowForward';
import DoneIcon from '@mui/icons-material/Done';
import type { AssignmentWithQuestions, Rubric, QuestionSubmissionAttempt, QuestionSubmissionDraft } from '@/lib/types';
import { useAuth } from '@/features/auth/AuthProvider';
import { DEFAULT_GRADING_OPTIONS } from '@/lib/api';

interface PlayerProps {
    assignmentId: string | null;
    onClose: () => void;
    onSubmitted?: (submission: { id: string; grade?: number; status?: string }) => void;
    previewMode?: boolean;
}

interface LinkedQuestion { id: string; title: string; difficulty: string; status: string; attempts: number; maxPoints: number; position: number; pointsOverride?: number | null }

export default function StudentAssignmentPlayer({ assignmentId, onClose, onSubmitted, previewMode = false }: PlayerProps) {

    const [assignment, setAssignment] = useState<AssignmentWithQuestions | null>(null);
    const [loading, setLoading] = useState(false);
    const [activeIdx, setActiveIdx] = useState(0);
    const [sql, setSql] = useState<string>('');
    const [grading, setGrading] = useState(false);
    const [grade, setGrade] = useState<number | null>(null);
    const [feedback, setFeedback] = useState<string[]>([]);
    const [rubric, setRubric] = useState<Rubric>({ syntax: 0, semantics: 0, results: 0 });
    const [tab, setTab] = useState(1); // 0 schema,1 editor
    const [questionDetail, setQuestionDetail] = useState<Partial<{ prompt: string; hints?: string; modelQueries?: string[]; modelSql?: string; initSql?: string; useDefaultGrading?: boolean; gradingOptions?: any }> | null>(null);
    const [qLoading, setQLoading] = useState(false);
    const { user, authFetch } = useAuth();
    const [submitting, setSubmitting] = useState(false);
    const [submitted, setSubmitted] = useState(false);
    const [isLocked, setIsLocked] = useState(false); // NEW
    const [submitMsg, setSubmitMsg] = useState<string | null>(null); // status message

    // parent submission record & guard
    const [submission, setSubmission] = useState<{ id: string; grade?: number; status?: string; assignment?: string; student?: string } | null>(null);
    const creatingSubmissionRef = useRef(false);
    const restoredRef = useRef(false); // track if we've restored the active question once
    const pendingDraftRef = useRef<null | { questionId: string; sql: string; grade: number | null; feedback: string[]; rubric: Rubric; status: string }>(null);

    // v2: namespace by submission id to avoid cross-submission pollution
    function draftStorageKey(qId: string) {
        const u = user?.name || 'anon';
        const a = assignment?.id || assignmentId || 'no-assignment';
        const s = submission?.id || 'pending';
        return `sqlgrader.v2.draft.${u}.${a}.${s}.${qId}`;
    }

    // Best-effort cleanup of legacy v1 keys when (re)starting an assignment
    const cleanupLegacyDrafts = useCallback(() => {
        if (typeof window === 'undefined') return;
        try {
            const u = user?.name || 'anon';
            const a = assignment?.id || assignmentId || 'no-assignment';
            // Remove old v1 keys
            const legacyPrefix = `sqlgrader.draft.${u}.${a}.`;
            for (let i = 0; i < localStorage.length; i++) {
                const k = localStorage.key(i) || '';
                if (k.startsWith(legacyPrefix)) {
                    localStorage.removeItem(k);
                }
            }
            // Optionally prune any leftover pending v2 keys for this assignment (different submission ids)
            const v2AssignPrefix = `sqlgrader.v2.draft.${u}.${a}.`;
            for (let i = 0; i < localStorage.length; i++) {
                const k = localStorage.key(i) || '';
                if (k.startsWith(v2AssignPrefix) && k.includes('.pending.')) {
                    localStorage.removeItem(k);
                }
            }
        } catch { /* ignore */ }
    }, [user?.name, assignment?.id, assignmentId]);

    async function ensureSubmissionImmediate() {
        if (previewMode) return null;
        if (isLocked) return submission;
        if (submission) return submission;
        if (!assignment || !user?.token) return null;
        if (creatingSubmissionRef.current) return submission;
        creatingSubmissionRef.current = true;
        try {
            const sRes = await authFetch('/api/submissions');
            if (sRes.ok) {
                const all = await sRes.json();
                const mine = (all || []).filter((s: any) => s.student === user.name && s.assignment === assignment.title);
                const final = mine.find((s: any) => s.status === 'Submitted' || s.status === 'Auto-graded' || s.status === 'Needs review');
                if (final) { setSubmission(final); setSubmitted(true); setIsLocked(true); return final; }
                const inProg = mine.find((s: any) => s.status === 'In Progress');
                if (inProg) { setSubmission(inProg); return inProg; }
            }
            const body = { student: user?.name || 'student', assignment: assignment.title, date: new Date().toISOString(), grade: 0, status: 'In Progress' };
            const res = await authFetch('/api/submissions', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
            if (res.ok) { const created = await res.json(); setSubmission(created); return created; }
        } catch { /* ignore */ } finally { creatingSubmissionRef.current = false; }
        return submission;
    }

    const currentQuestion: LinkedQuestion | null = useMemo(() => assignment?.questions?.sort((a, b) => a.position - b.position)[activeIdx] || null, [assignment, activeIdx]);
    const [questionDrafts, setQuestionDrafts] = useState<Record<string, QuestionSubmissionDraft>>({});
    const [attemptMap, setAttemptMap] = useState<Record<string, number>>({}); // questionId -> attempt
    // map questionId -> questionSubmissionId for history lookups
    const [qsIdByQuestion, setQsIdByQuestion] = useState<Record<string, string>>({});
    // per-question history cache
    const [historyMap, setHistoryMap] = useState<Record<string, { loading: boolean; attempts: QuestionSubmissionAttempt[]; error?: string }>>({});
    const [historyOpen, setHistoryOpen] = useState(false);
    const [showHint, setShowHint] = useState(false);

    const attemptsAllowed = (assignment as any)?.attemptsAllowed ?? 3;
    const attemptsUsed = currentQuestion ? (attemptMap[currentQuestion.id] || 0) : 0;
    const attemptsLeft = Math.max(0, (Number(attemptsAllowed) || 0) - attemptsUsed);
    // enable submit only on last question
    const totalQuestions = assignment?.questions?.length || 0;
    const isOnLastQuestion = totalQuestions > 0 && activeIdx === (totalQuestions - 1);

    const load = useCallback(async () => {
        if (!assignmentId) { setAssignment(null); return; }
        setLoading(true);
        try {
            if (!user?.token) return; // wait for auth
            const res = await authFetch(`/api/assignments?include=questions`);
            const data = await res.json();
            const found = (Array.isArray(data) ? data : []).find((a: AssignmentWithQuestions) => a.id === assignmentId) || null;
            setAssignment(found);
            setActiveIdx(0);
            setSql('');
            setGrade(null); setFeedback([]); setRubric({ syntax: 0, semantics: 0, results: 0 });
            setSubmission(null); setSubmitted(false); setIsLocked(false);
            setAttemptMap({});
            setQsIdByQuestion({});
            setHistoryMap({});
            setHistoryOpen(false);
            restoredRef.current = false; // allow initial restoration on this load
            // Cleanup any legacy local drafts for this assignment to avoid stale prefill
            cleanupLegacyDrafts();
        } catch { /* ignore */ } finally { setLoading(false); }
    }, [assignmentId, user?.token, authFetch]);

    useEffect(() => { load(); }, [load]);

    // After assignment + user loaded, check for existing submitted/graded submission to lock
    useEffect(() => {
        if (previewMode) return; // skip lock logic in preview mode
        async function checkLock() {
            if (!assignment || !user?.token) return;
            try {
                const sRes = await authFetch('/api/submissions');
                if (!sRes.ok) return; const all = await sRes.json();
                const existing = (all || []).find((s: any) => s.student === user.name && s.assignment === assignment.title && (s.status === 'Submitted' || s.status === 'Auto-graded' || s.status === 'Needs review'));
                if (existing) {
                    setSubmission(existing);
                    setSubmitted(true);
                    setIsLocked(true);
                }
            } catch { /* ignore */ }
        }
        checkLock();
    }, [assignment, user, previewMode]);

    // Ensure a parent submission exists (reuse existing where possible).
    useEffect(() => {
        if (previewMode) return; // skip submission creation in preview
        async function ensureParent() {
            if (!assignment || !user?.token || submission || creatingSubmissionRef.current) return;
            creatingSubmissionRef.current = true;
            try {
                const sRes = await authFetch('/api/submissions');
                if (sRes.ok) {
                    const all = await sRes.json();
                    const mine = (all || []).filter((s: any) => s.student === user.name && s.assignment === assignment.title);
                    // Treat any graded/final statuses as final (lock)
                    const final = mine.find((s: any) => s.status === 'Submitted' || s.status === 'Auto-graded' || s.status === 'Needs review');
                    if (final) { setSubmission(final); setSubmitted(true); setIsLocked(true); return; }
                    // Otherwise reuse existing in-progress
                    const inProg = mine.find((s: any) => s.status === 'In Progress');
                    if (inProg) { setSubmission(inProg); return; }
                }
                // If nothing to reuse and not locked, create a new in-progress record
                if (!isLocked) {
                    const body = { student: user?.name || 'preview_student', assignment: assignment.title, date: new Date().toISOString(), grade: 0, status: 'In Progress' };
                    const res = await authFetch('/api/submissions', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
                    if (res.ok) { const created = await res.json(); setSubmission(created); }
                }
            } catch { /* ignore */ } finally { creatingSubmissionRef.current = false; }
        }
        ensureParent();
    }, [assignment, user, submission, isLocked, previewMode]);

    // fetch detail of current question (prompt, hints, etc.)
    useEffect(() => {
        async function fetchDetail() {
            if (!currentQuestion) { setQuestionDetail(null); return; }
            setQLoading(true);
            try {
                if (!user?.token) return; // wait for auth
                const res = await authFetch(`/api/questions/${currentQuestion.id}`);
                if (res.ok) { setQuestionDetail(await res.json()); } else { setQuestionDetail(null); }
            } catch { setQuestionDetail(null); } finally { setQLoading(false); }
        }
        fetchDetail();
    }, [currentQuestion?.id, user?.token, authFetch]);

    // Load existing question_submissions drafts once parent submission exists
    useEffect(() => {
        if (previewMode) return; // no draft loading in preview
        async function loadDrafts() {
            if (!submission || !assignment || !user?.token) return;
            try {
                const res = await authFetch(`/api/question-submissions?submissionId=${encodeURIComponent(submission.id)}`);
                if (res.ok) {
                    const data = await res.json();
                    const map: Record<string, QuestionSubmissionDraft> = {};
                    const attempts: Record<string, number> = {};
                    const idMap: Record<string, string> = {};
                    (data || []).forEach((qs: any) => { map[qs.questionId] = { sql: qs.sql || '', grade: qs.grade ?? null, feedback: qs.feedback || [], rubric: (qs.rubric || null) as any }; attempts[qs.questionId] = qs.attempt || 0; idMap[qs.questionId] = qs.id; });
                    // Merge localStorage fallback drafts (v2 only, submission-scoped)
                    const sortedQs = [...(assignment?.questions || [])].sort((a: any, b: any) => a.position - b.position);
                    for (const q of sortedQs) {
                        try {
                            const key = draftStorageKey(q.id);
                            const raw = typeof window !== 'undefined' ? localStorage.getItem(key) : null;
                            if (!raw) continue;
                            const local = JSON.parse(raw);
                            // only adopt local if it matches this submission and server has no content
                            if (local && local.submissionId === submission.id && typeof local.sql === 'string') {
                                const existing = map[q.id];
                                const serverHasContent = !!(existing && typeof existing.sql === 'string' && existing.sql.trim().length > 0);
                                const localHasContent = (local.sql || '').trim().length > 0;
                                if (!serverHasContent && localHasContent) {
                                    map[q.id] = { sql: local.sql || '', grade: local.grade ?? null, feedback: Array.isArray(local.feedback) ? local.feedback : [], rubric: local.rubric || null } as any;
                                }
                            }
                        } catch { /* ignore */ }
                    }
                    setQuestionDrafts(map as Record<string, QuestionSubmissionDraft>);
                    setAttemptMap(attempts);
                    setQsIdByQuestion(idMap);
                    // On first load, pick the first question if locked/submitted, else pick the most recently worked question
                    if (!restoredRef.current && assignment?.questions?.length) {
                        if (isLocked) {
                            setActiveIdx(0);
                        } else {
                            const sorted = [...assignment.questions].sort((a, b) => a.position - b.position);
                            let pickedIdx = 0;
                            for (let i = 0; i < sorted.length; i++) {
                                const q = sorted[i];
                                const att = attempts[q.id] || 0;
                                const d = map[q.id];
                                const hasWork = att > 0 || (d && ((d.sql && d.sql.trim().length > 0) || d.grade != null));
                                if (hasWork) pickedIdx = i; // choose the furthest with work
                            }
                            setActiveIdx(pickedIdx);
                        }
                        restoredRef.current = true;
                    }
                }
            } catch { /* ignore */ }
        }
        loadDrafts();
    }, [submission, assignment, user, previewMode]);

    // When question changes, populate local state from draft map
    useEffect(() => {
        if (!currentQuestion) return;
        const draft = questionDrafts[currentQuestion.id];
        if (draft) {
            setSql(draft.sql);
            setGrade(draft.grade);
            setFeedback(draft.feedback);
            setRubric({
                syntax: typeof draft.rubric?.syntax === 'number' ? draft.rubric.syntax : 0,
                semantics: typeof draft.rubric?.semantics === 'number' ? draft.rubric.semantics : 0,
                results: typeof draft.rubric?.results === 'number' ? draft.rubric.results : 0,
                absent: draft.rubric?.absent
            });
        } else {
            setSql(''); setGrade(null); setFeedback([]); setRubric({ syntax: 0, semantics: 0, results: 0 });
        }
        // Close history when switching questions
        setHistoryOpen(false);
        setShowHint(false);
    }, [currentQuestion?.id, questionDrafts]);

    // Persist (create/upsert) a question submission
    async function persistQuestionSubmission(opts: { question: LinkedQuestion; sqlText: string; gradeVal: number | null; feedbackVals: string[]; rubricVal: Rubric; status?: string; incrementAttempt?: boolean }) {
        if (previewMode) return; // skip persistence in preview
        if (isLocked) return;
        if (!submission || !assignment || !user) return; // nothing to do yet
        const { question, sqlText, gradeVal, feedbackVals, rubricVal, status, incrementAttempt } = opts;
        try {
            const resp = await authFetch('/api/question-submissions', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    submissionId: submission.id,
                    assignmentId: assignment.id,
                    questionId: question.id,
                    student: user.name,
                    sql: sqlText,
                    grade: gradeVal,
                    status: status || (gradeVal != null ? 'Auto-graded' : 'Draft'),
                    rubric: gradeVal != null ? rubricVal : null,
                    feedback: feedbackVals,
                    incrementAttempt: !!incrementAttempt
                })
            });
            if (resp.ok) {
                const saved = await resp.json();
                if (saved?.id) setQsIdByQuestion(m => ({ ...m, [question.id]: saved.id }));
            }
            // update local cache
            setQuestionDrafts(d => ({ ...d, [question.id]: { sql: sqlText, grade: gradeVal, feedback: feedbackVals, rubric: gradeVal != null ? (rubricVal as any) : null } }));
        } catch { /* ignore */ }
    }

    // Load attempt history for given question id
    async function loadHistoryFor(questionId: string) {
        const qsId = qsIdByQuestion[questionId];
        if (!qsId) return;
        setHistoryMap(m => ({ ...m, [questionId]: { ...(m[questionId] || { attempts: [] }), loading: true, error: undefined } }));
        try {
            const res = await authFetch(`/api/question-submissions?historyOf=${qsId}`);
            if (!res.ok) throw new Error('Failed to load attempt history');
            const attempts = await res.json();
            setHistoryMap(m => ({ ...m, [questionId]: { loading: false, attempts } }));
        } catch (e: any) {
            setHistoryMap(m => ({ ...m, [questionId]: { loading: false, attempts: [], error: e?.message || 'Failed' } }));
        }
    }

    async function runGrade() {
        if (!currentQuestion || isLocked) return;
        // Enforce attempts allowed (if 1, grading disabled)
        if ((Number(attemptsAllowed) || 0) <= 1) return;
        // Also block if no attempts left
        if (attemptsLeft <= 0) return;
        setGrading(true);
        try {
            // Fetch freshest details for modelQueries & assignment initSql
            const qRes = await authFetch(`/api/questions/${currentQuestion.id}`);
            const qDetail = qRes.ok ? await qRes.json() : null;
            const modelQueries: string[] = (Array.isArray(qDetail?.modelQueries) && qDetail.modelQueries.length > 0)
                ? qDetail.modelQueries
                : ((qDetail?.modelSql && String(qDetail.modelSql).trim()) ? [qDetail.modelSql] : []);
            // Per-question init SQL (no global assignment-level initializer)
            const initSql = qDetail?.initSql || '';
            const attempt = (attemptMap[currentQuestion.id] || 0) + 1;

            // Build autograder payload mirroring batch grader fields for a single query
            const options = (qDetail && qDetail.useDefaultGrading === false && qDetail.gradingOptions) ? qDetail.gradingOptions : DEFAULT_GRADING_OPTIONS;
            const payload = {
                sql_data: initSql,
                sql_create_data: initSql,
                queries: [[encodeURIComponent(user?.name || 'student'), attempt, '1', sql]],
                model_queries: modelQueries,
                syntax: parseInt(options.syntaxSensitivity, 10) || 0,
                semantics: parseInt(options.semanticsSensitivity, 10) || 0,
                results: parseInt(options.resultsSensitivity, 10) || 0,
                prop_order: parseInt(options.evaluationPriority, 10) || 0,
                edit_dist: parseInt(options.textEditDistance, 10) || 0,
                tree_dist: parseInt(options.treeEditDistance, 10) || 0,
                check_order: options.checkOrder ? 1 : 0,
                auto_db: options.autoDB ? 1 : 0,
                num_db: parseInt(options.numberOfDBs, 10) || 0,
                dbname: options.dbName || '',
                use_postgresql: !!options.use_postgresql,
            };
            let gradeResult: { grade: number; feedback: string[]; rubric: Rubric } | null = null;
            // Helper: fetch with timeout using AbortController
            const fetchWithTimeout = async (url: string, options: RequestInit, timeoutMs: number) => {
                const controller = new AbortController();
                const timer = setTimeout(() => controller.abort(), timeoutMs);
                try {
                    const res = await fetch(url, { ...options, signal: controller.signal });
                    return res;
                } finally {
                    clearTimeout(timer);
                }
            };
            try {
                const resp = await fetchWithTimeout(`${process.env.NEXT_PUBLIC_SOCOLES_API_URL || 'http://localhost:5000'}${process.env.NEXT_PUBLIC_SOCOLES_GRADE_PATH || '/grade-queries'}`, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload),
                }, 15000);
                if (resp.ok) {
                    const data = await resp.json();
                    // Assume backend returns array or object; adapt heuristically
                    if (Array.isArray(data) && data.length) {
                        const first = data[0] as any;
                        const g = Number(first.Score || first.grade || first.Grade || 0);
                        const rawFb = (first.Feedback || first.feedback || '') as string;
                        const fb = Array.isArray(rawFb) ? rawFb : String(rawFb).split(/;|\n/).filter((s: string) => s.trim());
                        const r = first.Rubric || first.rubric;
                        let rb: Rubric = { syntax: 0, semantics: 0, results: 0 } as any;
                        if (r && typeof r === 'object') {
                            rb = {
                                syntax: typeof r.syntax === 'number' ? r.syntax : null,
                                semantics: typeof r.semantics === 'number' ? r.semantics : null,
                                results: typeof r.results === 'number' ? r.results : null,
                                absent: {
                                    syntax: typeof r.syntax !== 'number',
                                    semantics: typeof r.semantics !== 'number',
                                    results: typeof r.results !== 'number'
                                }
                            };
                        }
                        gradeResult = { grade: g, feedback: fb, rubric: rb } as any;
                    } else if (data && typeof data === 'object') {
                        const anyData = data as any;
                        const g = Number(anyData.grade || anyData.score || anyData.Grade || anyData.Score || 0);
                        const fb = Array.isArray(anyData.feedback) ? anyData.feedback : [];
                        const r = (anyData as any).Rubric || (anyData as any).rubric;
                        let rb: Rubric = { syntax: 0, semantics: 0, results: 0 } as any;
                        if (r && typeof r === 'object') {
                            rb = {
                                syntax: typeof r.syntax === 'number' ? r.syntax : null,
                                semantics: typeof r.semantics === 'number' ? r.semantics : null,
                                results: typeof r.results === 'number' ? r.results : null,
                                absent: {
                                    syntax: typeof r.syntax !== 'number',
                                    semantics: typeof r.semantics !== 'number',
                                    results: typeof r.results !== 'number'
                                }
                            };
                        }
                        gradeResult = { grade: g, feedback: fb, rubric: rb } as any;
                    }
                }
            } catch { /* network/timeout failure -> fallback placeholder below */ }
            if (!gradeResult) {
                // Fallback placeholder if autograder not reachable
                const newGrade = 0; // 0..1 with 2 decimals
                gradeResult = { grade: newGrade, feedback: ['Autograder unavailable or timed out.'], rubric: { syntax: 0, semantics: 0, results: 0, absent: { syntax: true, semantics: true, results: true } } as any } as any;
            }
            const finalResult = gradeResult as { grade: number; feedback: string[]; rubric: Rubric };
            const { grade: gVal, feedback: fbVal, rubric: rbVal } = finalResult;
            setGrade(gVal);
            setFeedback(fbVal);
            setRubric(rbVal as Rubric);
            // Only increment attempt if grading genuinely succeeded (we got a real response)
            const succeeded = !!gradeResult && !String(gradeResult.feedback?.[0] || '').includes('unavailable or timed out');
            if (succeeded) {
                setAttemptMap(m => ({ ...m, [currentQuestion.id]: attempt }));
            }
            if (!previewMode) {
                await persistQuestionSubmission({ question: currentQuestion, sqlText: sql, gradeVal: gVal, feedbackVals: fbVal, rubricVal: rbVal as Rubric, status: 'Auto-graded', incrementAttempt: succeeded });
                // If history pane is open, refresh it
                if (historyOpen) await loadHistoryFor(currentQuestion.id);
            }
        } finally { setGrading(false); }
    }

    // Save draft (no grade) before moving away
    async function saveDraftFor(question: LinkedQuestion | null) {
        if (isLocked) return;
        if (!question) return;
        if (!sql.trim() && grade == null) return; // nothing meaningful
        if (!submission) {
            pendingDraftRef.current = { questionId: question.id, sql, grade, feedback, rubric, status: grade != null ? 'Auto-graded' : 'Draft' };
            // Save pending local draft with metadata to avoid cross-submission reuse
            try {
                if (typeof window !== 'undefined') {
                    const u = user?.name || 'anon';
                    const a = assignment?.id || assignmentId || 'no-assignment';
                    const payload = { ...pendingDraftRef.current, submissionId: null, assignmentId: a, user: u, ts: Date.now() };
                    localStorage.setItem(draftStorageKey(question.id), JSON.stringify(payload));
                }
            } catch { /* ignore */ }
            const sub = await ensureSubmissionImmediate();
            if (sub) {
                await persistQuestionSubmission({ question, sqlText: sql, gradeVal: grade, feedbackVals: feedback, rubricVal: rubric, status: grade != null ? 'Auto-graded' : 'Draft' });
                try { if (typeof window !== 'undefined') localStorage.removeItem(draftStorageKey(question.id)); } catch { /* ignore */ }
                pendingDraftRef.current = null;
            }
            return;
        }
        await persistQuestionSubmission({ question, sqlText: sql, gradeVal: grade, feedbackVals: feedback, rubricVal: rubric, status: grade != null ? 'Auto-graded' : 'Draft' });
    }

    // Debounced autosave when the student edits SQL
    useEffect(() => {
        if (previewMode || isLocked) return; // don't autosave in preview or when locked
        const q = currentQuestion;
        if (!q) return;
        const currentDraft = questionDrafts[q.id];
        // Only autosave meaningful text changes to avoid noisy writes
        const changed = (sql || '').trim() !== (currentDraft?.sql || '').trim();
        if (!changed) return;
        const handle = setTimeout(() => {
            if (!submission) {
                pendingDraftRef.current = { questionId: q.id, sql, grade, feedback, rubric, status: grade != null ? 'Auto-graded' : 'Draft' };
                try {
                    if (typeof window !== 'undefined') {
                        const u = user?.name || 'anon';
                        const a = assignment?.id || assignmentId || 'no-assignment';
                        const payload = { ...pendingDraftRef.current, submissionId: null, assignmentId: a, user: u, ts: Date.now() };
                        localStorage.setItem(draftStorageKey(q.id), JSON.stringify(payload));
                    }
                } catch { /* ignore */ }
                void ensureSubmissionImmediate().then((sub) => {
                    if (sub) {
                        void persistQuestionSubmission({ question: q, sqlText: sql, gradeVal: grade, feedbackVals: feedback, rubricVal: rubric, status: grade != null ? 'Auto-graded' : 'Draft' }).then(() => {
                            try { if (typeof window !== 'undefined') localStorage.removeItem(draftStorageKey(q.id)); } catch { /* ignore */ }
                            pendingDraftRef.current = null;
                        });
                    }
                });
            } else {
                void persistQuestionSubmission({
                    question: q,
                    sqlText: sql,
                    gradeVal: grade,
                    feedbackVals: feedback,
                    rubricVal: rubric,
                    status: grade != null ? 'Auto-graded' : 'Draft'
                });
            }
        }, 1200);
        return () => clearTimeout(handle);
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [sql, currentQuestion?.id, previewMode, isLocked, questionDrafts, submission]);

    // Ensure we persist work before closing the player
    const handleClose = useCallback(async () => {
        try {
            await ensureSubmissionImmediate();
            await saveDraftFor(currentQuestion);
        } finally {
            onClose();
        }
    }, [currentQuestion, onClose, sql, grade, feedback, rubric]);

    // Flush any queued pending draft once a submission becomes available
    useEffect(() => {
        const pending = pendingDraftRef.current;
        if (!pending || !submission || previewMode || isLocked) return;
        const qid = pending.questionId;
        const q = assignment?.questions?.find((x: any) => x.id === qid) as any;
        if (!q) return;
        void persistQuestionSubmission({ question: q, sqlText: pending.sql, gradeVal: pending.grade, feedbackVals: pending.feedback, rubricVal: pending.rubric, status: pending.status }).then(() => {
            try { if (typeof window !== 'undefined') localStorage.removeItem(draftStorageKey(qid)); } catch { /* ignore */ }
            pendingDraftRef.current = null;
        });
    }, [submission, previewMode, isLocked]);

    async function submitAnswer() {
        if (previewMode) return; // disabled in preview
        if (isLocked) return; // already locked
        if (!assignment || !currentQuestion || !user || submitting || !submission) return;
        // Enforce: can only submit on last question
        const tq = assignment?.questions?.length || 0;
        if (activeIdx !== (tq - 1)) return;
        setSubmitting(true);
        setSubmitMsg('Submitting…');
        try {
            // Save current question
            await saveDraftFor(currentQuestion);
            setSubmitMsg('Submitting… starting auto-grader');
            // Start async grading job
            const start = await authFetch('/api/grade-submission', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ submissionId: submission.id, async: true }) });
            if (!start.ok) throw new Error('Failed to start grading');
            const started = await start.json();
            const jobId: string | undefined = started?.jobId;
            if (!jobId) throw new Error('No job id');
            // Poll job until done (120s cap)
            const deadline = Date.now() + 120_000;
            let final: any = null;
            while (Date.now() < deadline) {
                await new Promise(r => setTimeout(r, 1000));
                const poll = await authFetch(`/api/grade-submission?job=${encodeURIComponent(jobId)}`);
                if (!poll.ok) continue;
                const body = await poll.json();
                if (body.status === 'succeeded') { final = body.result; break; }
                if (body.status === 'failed') { throw new Error(body.error || 'Grading failed'); }
                // else queued/running -> continue
                setSubmitMsg('Grading in progress…');
            }
            if (!final) {
                setSubmitMsg('Still processing in background. You will see the results shortly.');
                // Do not lock immediately to allow UI to refresh later, but we can mark as submitted
                setSubmitted(true);
                setIsLocked(true);
                return;
            }
            // Update submission state & lock
            setSubmitted(true);
            setIsLocked(true);
            setSubmitMsg(final.status === 'Needs review' ? 'Completed with issues. Marked Needs review.' : 'Auto-grading complete.');
            if (onSubmitted) onSubmitted({ ...submission, grade: final?.submission?.grade ?? 0, status: final?.status || 'Auto-graded' });
            // Optionally close after a short delay
            setTimeout(() => { onClose(); }, 1000);
        } catch (e: any) {
            setSubmitMsg(e?.message || 'Failed to submit');
        } finally { setSubmitting(false); }
    }

    function next() {
        saveDraftFor(currentQuestion);
        setActiveIdx(i => Math.min(i + 1, (assignment?.questions?.length || 1) - 1));
        setGrade(null); setFeedback([]); setRubric({ syntax: 0, semantics: 0, results: 0 }); setSql('');
    }
    function prev() {
        saveDraftFor(currentQuestion);
        setActiveIdx(i => Math.max(i - 1, 0));
        setGrade(null); setFeedback([]); setRubric({ syntax: 0, semantics: 0, results: 0 }); setSql('');
    }

    if (!assignmentId) return null;
    if (loading) return <Card sx={{ p: 3 }}><Typography>Loading assignment…</Typography></Card>;
    if (!assignment) return <Card sx={{ p: 3 }}><Typography>Assignment not found.</Typography><Button sx={{ mt: 1 }} onClick={onClose}>Close</Button></Card>;

    // Determine denominator for display purposes
    const gradeDenom = typeof grade === 'number' ? (grade <= 1 ? 1 : 10) : null;

    return (
        <Card sx={{ display: 'flex', flexDirection: 'column', position: 'relative' }}>
            {previewMode && (
                <Box sx={{ position: 'absolute', top: 8, right: 8 }}>
                    <Chip size='small' color='warning' label='Preview Mode' />
                </Box>
            )}
            <CardHeader
                title={<Typography variant='subtitle1'>{assignment.title}</Typography>}
                subheader={<Typography variant='caption' color='text.secondary'>Question {activeIdx + 1} of {assignment.questions.length}{previewMode && ' • Preview'}</Typography>}
                action={!previewMode ? <Button size='small' onClick={handleClose}>Close</Button> : undefined}
            />
            <CardContent sx={{ display: 'grid', gap: 2, gridTemplateColumns: { lg: '1fr 340px' } }}>
                <Box>
                    <Typography variant='subtitle2' sx={{ mb: .5 }}>{currentQuestion?.title}</Typography>
                    <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap', mb: 1 }}>
                        {currentQuestion?.difficulty && (
                            <DifficultyChip value={currentQuestion.difficulty} size='small' />
                        )}
                        {currentQuestion?.pointsOverride != null ? <Chip size='small' label={`${currentQuestion.pointsOverride} pts`} color='primary' /> : <Chip size='small' label={`${currentQuestion?.maxPoints} pts`} />}
                        <Chip size='small' label={`Attempts left: ${attemptsLeft}`} color={attemptsLeft > 0 ? 'default' : 'warning'} />
                    </Box>
                    {!!questionDetail && (
                        <Box sx={{ mb: 1 }}>
                            {qLoading && <Typography variant='caption' color='text.secondary'>Loading question…</Typography>}
                            {!qLoading && (
                                <PromptWithHint
                                    prompt={String(questionDetail.prompt || 'No prompt provided.')}
                                    hint={String(questionDetail?.hints || '')}
                                    showHint={showHint}
                                    onToggle={() => setShowHint(s => !s)}
                                    promptKey={`prompt-${currentQuestion?.id || 'none'}`}
                                    hintKeyPrefix={`hint-${currentQuestion?.id || 'none'}`}
                                />
                            )}
                        </Box>
                    )}
                    <Tabs value={tab} onChange={(_, v) => setTab(v)} sx={{ mb: 1 }}>
                        <Tab label='Schema' />
                        <Tab label='Editor' />
                    </Tabs>
                    {tab === 0 && (
                        <Box sx={{ color: 'text.secondary', fontSize: 14 }}>
                            <p style={{ margin: 0 }}>Schema</p>
                            <SchemaPreview sql={String(questionDetail?.initSql || '')} />
                        </Box>
                    )}
                    {tab === 1 && (
                        <SqlEditor value={sql} onChange={setSql} placeholder='-- Write your SQL here' readOnly={isLocked} minRows={10} />
                    )}
                </Box>
                <Box sx={{ display: 'grid', gap: 2 }}>
                    <Card variant='outlined'>
                        <CardHeader title={<Typography variant='subtitle2'>Auto-Grader</Typography>} />
                        <CardContent>
                            <Box sx={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 1.25 }}>
                                <Button onClick={runGrade} disabled={grading || !sql.trim() || (Number(attemptsAllowed) || 0) <= 1 || attemptsLeft <= 0} startIcon={<PlayArrowIcon />} variant='contained'>{grading ? 'Grading…' : 'Run'}</Button>
                                {!previewMode && <Button variant='outlined' startIcon={<DoneIcon />} disabled={!isOnLastQuestion || submitted || submitting || isLocked} onClick={submitAnswer}>{submitted ? 'Submitted' : 'Submit'}</Button>}
                            </Box>
                            {previewMode && <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mt: .75 }}>Preview: grading not persisted.</Typography>}
                            <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mt: .75 }}>Attempts allowed: {Number(attemptsAllowed) || 0}. Used: {attemptsUsed}. Left: {attemptsLeft}.</Typography>
                            <Box sx={{ mt: 2 }}>
                                {isLocked && <Typography variant='caption' color='error' sx={{ display: 'block', mb: .5 }}>Submission locked (already submitted).</Typography>}
                                {!!submitMsg && <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mb: .5 }}>{submitMsg}</Typography>}
                                <GradeDisplay grade={typeof grade === 'number' ? grade : null} denom={gradeDenom} />
                            </Box>
                            <Divider sx={{ my: 1.5 }} />
                            <RubricBars rubric={rubric} />
                            <Box sx={{ mt: 2 }}>
                                <Typography variant='body2' color='text.secondary'>Feedback</Typography>
                                <FeedbackList feedback={feedback} emptyText='No feedback yet.' />
                            </Box>
                            <Divider sx={{ my: 1.5 }} />
                            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                                <Button size='small' variant='text' onClick={async () => {
                                    const qid = currentQuestion?.id;
                                    if (!qid) return;
                                    const opened = !historyOpen;
                                    setHistoryOpen(opened);
                                    if (opened && !historyMap[qid]?.attempts?.length) {
                                        await loadHistoryFor(qid);
                                    }
                                }}>Attempt history</Button>
                                {historyOpen && (historyMap[currentQuestion?.id || '']?.loading ? <Typography variant='caption' color='text.secondary' sx={{ display: 'inline-flex', alignItems: 'center', gap: .75 }}><CircularProgress size={12} /> Loading…</Typography> : null)}
                                {historyOpen && historyMap[currentQuestion?.id || '']?.error && <Typography variant='caption' color='error'>{historyMap[currentQuestion?.id || '']?.error}</Typography>}
                            </Box>
                            <Collapse in={historyOpen} unmountOnExit>
                                <Box sx={{ mt: 1.25, display: 'grid', gap: 1 }}>
                                    {(() => {
                                        const qid = currentQuestion?.id || '';
                                        const attempts = historyMap[qid]?.attempts || [];
                                        if (!attempts.length && !historyMap[qid]?.loading) return <Typography variant='caption' color='text.secondary'>No attempts yet.</Typography>;
                                        return attempts.map((a) => (
                                            <Box key={a.id} sx={{ p: 1, border: '1px solid', borderColor: 'divider', borderRadius: 1 }}>
                                                <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', flexWrap: 'wrap', gap: .5 }}>
                                                    <Typography variant='subtitle2'>Attempt {a.attempt}</Typography>
                                                    <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
                                                        <Chip size='small' label={`Grade: ${typeof a.grade === 'number' ? a.grade.toFixed(2) : '—'}`} />
                                                        <Typography variant='caption' color='text.secondary'>{a.createdAt ? formatDateTimeDDMMYYYYHHmm(a.createdAt) : ''}</Typography>
                                                    </Box>
                                                </Box>
                                                <Typography variant='caption' color='text.secondary'>SQL</Typography>
                                                <Box sx={{ mt: .5, p: .75, bgcolor: 'background.default', borderRadius: 1, fontFamily: 'ui-monospace,monospace' }}>
                                                    <Typography variant='body2' sx={{ whiteSpace: 'pre-wrap' }}>{a.sql || '--'}</Typography>
                                                </Box>
                                                <Box sx={{ mt: 1 }}>
                                                    <Typography variant='caption' color='text.secondary'>Feedback</Typography>
                                                    {Array.isArray(a.feedback) && a.feedback.length ? (
                                                        <Box component='ul' sx={{ pl: 2.25, my: .25, listStyle: 'none' }}>
                                                            {a.feedback.map((f, i) => <li key={i}><Typography variant='body2' sx={{ whiteSpace: 'pre-line' }}>{f}</Typography></li>)}
                                                        </Box>
                                                    ) : (
                                                        <Typography variant='caption' color='text.secondary'>No feedback.</Typography>
                                                    )}
                                                </Box>
                                            </Box>
                                        ));
                                    })()}
                                </Box>
                            </Collapse>
                        </CardContent>
                    </Card>
                    <Card variant='outlined'>
                        <CardContent>
                            <Box sx={{ display: 'flex', gap: 1, justifyContent: 'space-between' }}>
                                <Button size='small' startIcon={<ArrowBackIcon />} disabled={activeIdx === 0} onClick={prev}>Previous</Button>
                                <Button size='small' endIcon={<ArrowForwardIcon />} disabled={activeIdx === (assignment.questions.length - 1)} onClick={next}>Next</Button>
                            </Box>
                            <Typography variant='caption' color='text.secondary' sx={{ display: 'block', mt: 1 }}>Navigate through questions. {submission ? `Submission: ${submission.id.substring(0, 8)}` : 'Creating submission…'}{isLocked && ' • Locked'}</Typography>
                        </CardContent>
                    </Card>
                </Box>
            </CardContent>
            <CardActions sx={{ justifyContent: 'space-between', flexWrap: 'wrap', gap: 1 }}>
                <Typography variant='caption' color='text.secondary'>Progress {activeIdx + 1}/{assignment.questions.length}{submitted && !previewMode && ' • Submitted'}{isLocked && !previewMode && ' • Locked'}{previewMode && ' • Preview only'}</Typography>
            </CardActions>
        </Card>
    );
}
