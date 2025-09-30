'use client';
import { useMemo, useState, useEffect } from 'react';
import type { ElementType } from 'react';
import {
    Box, Card, CardHeader, CardContent, CardActions, Typography, Chip, Button, Divider
} from '@mui/material';
import AssignmentTurnedInIcon from '@mui/icons-material/AssignmentTurnedIn';
import TimerIcon from '@mui/icons-material/Timer';
import CheckCircleIcon from '@mui/icons-material/CheckCircle';
import VisibilityIcon from '@mui/icons-material/Visibility';

import { simulateCppGrade } from '@/lib/mock';
import { alpha } from '@mui/material/styles';
import type { Theme } from '@mui/material/styles';
import EditorDialog from './EditorDialog';
import StudentSubmissions from './StudentSubmissions';
import StudentProfile from './StudentProfile';
import StudentAssignmentPlayer from './StudentAssignmentPlayer';
import { useAuth } from '@/features/auth/AuthProvider';
import PageCard from '@/components/PageCard';

interface AssignmentApi { id: string; title: string; course: string; difficulty: string; points: number; due: string; tags: string[] }
interface SubmissionApi { id: string; student: string; assignment: string; date: string; grade: number; status: string }

type Tone = 'primary' | 'secondary' | 'success' | 'info' | 'warning' | 'error';

// Parse common due date formats, including DD-MM-YYYY (e.g., 18-09-2025)
function parseDueMs(input: string): number {
    if (!input) return NaN;
    const t = Date.parse(input);
    if (!Number.isNaN(t)) return t;
    const m = input.trim().match(/^(\d{1,2})-(\d{1,2})-(\d{2,4})$/);
    if (m) {
        const day = parseInt(m[1], 10);
        const month = parseInt(m[2], 10);
        let year = parseInt(m[3], 10);
        if (year < 100) year += 2000;
        const dt = new Date(year, Math.max(0, month - 1), day);
        return dt.getTime();
    }
    return NaN;
}

function StatCard({ icon: Icon, label, value, postfix, tone }: { icon: ElementType; label: string; value: number | string; postfix?: string; tone?: Tone }) {
    const isSuccess = Icon === CheckCircleIcon || /finished/i.test(label);
    const effTone: Tone | undefined = tone ?? (isSuccess ? 'success' : undefined);
    return (
        <Card>
            <CardContent sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
                <Box
                    sx={{
                        p: 0.75,
                        borderRadius: 2,
                        border: '1px solid',
                        bgcolor: effTone
                            ? ((t: Theme) => alpha(
                                effTone === 'primary' ? t.palette.primary.main :
                                    effTone === 'secondary' ? t.palette.secondary.main :
                                        effTone === 'success' ? t.palette.success.main :
                                            effTone === 'info' ? t.palette.info.main :
                                                effTone === 'warning' ? t.palette.warning.main :
                                                    t.palette.error.main,
                                0.1
                            ))
                            : 'rgba(15,23,42,0.06)',
                        borderColor: effTone ? `${effTone}.light` : 'rgba(15,23,42,0.12)'
                    }}
                >
                    <Icon fontSize="small" color={effTone} />
                </Box>
                <Box>
                    <Typography variant="caption" color="text.secondary">{label}</Typography>
                    <Typography variant="h5" sx={{ lineHeight: 1, mt: 0.3 }}>
                        {value}{postfix ? <Typography component="span" variant="body2" color="text.secondary" sx={{ ml: .5 }}>{postfix}</Typography> : null}
                    </Typography>
                </Box>
            </CardContent>
        </Card>
    );
}

function AssignmentsGrid({
    openEditor,
    assignments,
    submittedTitles,
    inProgressTitles,
    highlightOverdueBorder,
}: {
    openEditor: (seedSql?: string, assignmentId?: string) => void;
    assignments: AssignmentApi[];
    submittedTitles: Set<string>;
    inProgressTitles?: Set<string>;
    highlightOverdueBorder?: boolean;
}) {
    return (
        <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { md: 'repeat(2,1fr)', xs: '1fr' } }}>
            {assignments.map((a) => (
                <Box key={a.id}>
                    {(() => {
                        const dueMs = parseDueMs(a.due);
                        const overdue = !submittedTitles.has(a.title) && !Number.isNaN(dueMs) && dueMs < Date.now();
                        return (
                            <Card sx={{
                                border: highlightOverdueBorder && overdue ? '1px solid' : undefined,
                                borderColor: highlightOverdueBorder && overdue ? 'error.main' : undefined,
                            }}>
                                <CardHeader
                                    title={<Typography variant="subtitle1">{a.title}</Typography>}
                                    subheader={<Typography variant="caption" color="text.secondary">{a.course}</Typography>}
                                    action={<Chip size="small" label={a.difficulty} variant="outlined" />}
                                    sx={{ pb: 0.5 }}
                                />
                                <CardContent sx={{ pt: 1.5 }}>
                                    <Box sx={{ display: 'flex', gap: 1, flexWrap: 'wrap', mb: 1 }}>
                                        {a.tags?.map((t) => <Chip key={t} size="small" label={t} variant="outlined" />)}
                                    </Box>
                                    <Typography variant="body2" color="text.secondary">Due: {a.due}</Typography>
                                </CardContent>
                                <CardActions sx={{ justifyContent: 'space-between' }}>
                                    <Typography variant="body2"><b>{a.points}</b> pts</Typography>
                                    {submittedTitles.has(a.title) ? (
                                        <Button size="small" variant="outlined" startIcon={<VisibilityIcon />} onClick={() => openEditor(undefined, a.id)}>View</Button>
                                    ) : inProgressTitles?.has(a.title) ? (
                                        <Button size="small" variant="contained" onClick={() => openEditor(undefined, a.id)}>Continue</Button>
                                    ) : (
                                        <Button size="small" variant="contained" onClick={() => openEditor(`-- ${a.title}\nSELECT * FROM Book;`, a.id)}>Start</Button>
                                    )}
                                </CardActions>
                            </Card>
                        );
                    })()}
                </Box>
            ))}
        </Box>
    );
}

export default function StudentArea({ active }: { active: string }) {
    // Editor state shared so grade/rubric persist across navigation
    const [sql, setSql] = useState<string>("SELECT *\nFROM Book\nWHERE author = 'Pratchett';");
    const [grading, setGrading] = useState(false);
    const [grade, setGrade] = useState<number | null>(null);
    const [feedback, setFeedback] = useState<string[]>([]);
    const [rubric, setRubric] = useState<{ syntax: number | null; semantics: number | null; results: number | null; absent?: { syntax?: boolean; semantics?: boolean; results?: boolean } }>({ syntax: 0, semantics: 0, results: 0 });
    const [editorOpen, setEditorOpen] = useState(false);
    const [assignments, setAssignments] = useState<AssignmentApi[]>([]);
    const [submissions, setSubmissions] = useState<SubmissionApi[]>([]);
    const [loading, setLoading] = useState(false);
    const [playerAssignmentId, setPlayerAssignmentId] = useState<string | null>(null);
    const { user, authFetch } = useAuth();

    useEffect(() => {
        if (!user?.token) return; // wait for auth token
        async function load() {
            setLoading(true);
            try {
                const [aRes, sRes] = await Promise.all([
                    authFetch('/api/assignments'),
                    authFetch('/api/submissions'),
                ]);
                const aData = await aRes.json();
                const sData = await sRes.json();
                setAssignments(Array.isArray(aData) ? aData : []);
                setSubmissions(Array.isArray(sData) ? sData : []);
            } catch { /* ignore for now */ }
            finally { setLoading(false); }
        }
        void load();
    }, [user?.token, authFetch]);

    // Treat 'Submitted', 'Auto-graded', and 'Needs review' as finished
    const submittedTitles = useMemo(() => {
        const set = new Set<string>();
        const me = user?.name;
        (submissions || []).forEach((s) => {
            if (s.student === me && (s.status === 'Submitted' || s.status === 'Auto-graded' || s.status === 'Needs review')) set.add(s.assignment);
        });
        return set;
    }, [submissions, user?.name]);

    // Categorize assignments for dashboard
    const categorized = useMemo(() => {
        const me = user?.name;
        const byTitle = new Map<string, SubmissionApi[]>();
        (submissions || []).forEach(s => {
            if (s.student !== me) return;
            if (!byTitle.has(s.assignment)) byTitle.set(s.assignment, []);
            byTitle.get(s.assignment)!.push(s);
        });
        const now = Date.now();
        function dueTime(d: string) { const t = parseDueMs(d); return Number.isNaN(t) ? Infinity : t; }
        const finishedTitles = new Set<string>();
        const inProgressTitles = new Set<string>();
        const overdueTitles = new Set<string>();

        assignments.forEach(a => {
            const subs = byTitle.get(a.title) || [];
            const hasFinished = subs.some(s => s.status === 'Submitted' || s.status === 'Auto-graded' || s.status === 'Needs review');
            const hasInProgress = subs.some(s => s.status === 'In Progress');
            const isOverdue = dueTime(a.due) < now && !hasFinished;
            if (hasFinished) finishedTitles.add(a.title);
            else if (hasInProgress) inProgressTitles.add(a.title);
            if (isOverdue) overdueTitles.add(a.title);
        });

        const finished = assignments.filter(a => finishedTitles.has(a.title));
        const inProgress = assignments.filter(a => inProgressTitles.has(a.title) && !overdueTitles.has(a.title));
        const overdue = assignments.filter(a => overdueTitles.has(a.title));
        const pending = assignments.filter(a => !finishedTitles.has(a.title) && !inProgressTitles.has(a.title) && !overdueTitles.has(a.title));

        return { finished, inProgress, overdue, pending, finishedTitles, inProgressTitles };
    }, [assignments, submissions, user?.name]);

    const nearestDueDays = useMemo(() => {
        const now = Date.now();
        let minDays = Infinity;
        assignments.forEach(a => {
            // Skip finished assignments
            if (categorized.finishedTitles.has(a.title)) return;
            const t = parseDueMs(a.due);
            if (Number.isNaN(t)) return;
            const days = Math.ceil((t - now) / (1000 * 60 * 60 * 24));
            if (days < minDays) minDays = days;
        });
        return minDays === Infinity ? 0 : minDays;
    }, [assignments, categorized.finishedTitles]);

    const nearestDueTone: Tone = useMemo(() => {
        const d = nearestDueDays;
        if (d < 3) return 'error';
        if (d <= 7) return 'warning';
        return 'info';
    }, [nearestDueDays]);

    async function runGrade() {
        setGrading(true);
        const res = await simulateCppGrade(sql);
        setGrade(res.grade);
        setFeedback(res.feedback);
        // Map legacy or backend rubric to new fields
        if (res.rubric && typeof res.rubric === 'object') {
            const anyR: any = res.rubric;
            const mapped = {
                syntax: (typeof anyR.syntax === 'number' ? anyR.syntax : (typeof anyR.correctness === 'number' ? anyR.correctness / 100 : 0)),
                semantics: (typeof anyR.semantics === 'number' ? anyR.semantics : (typeof anyR.style === 'number' ? anyR.style / 100 : 0)),
                results: (typeof anyR.results === 'number' ? anyR.results : (typeof anyR.efficiency === 'number' ? anyR.efficiency / 100 : 0)),
                absent: anyR.absent || {}
            };
            setRubric(mapped);
        } else {
            setRubric({ syntax: 0, semantics: 0, results: 0 });
        }
        setGrading(false);
    }

    const header = (title: string, hint?: string) => (
        <Box sx={{ mb: 1.5 }}>
            <Typography variant="h6">{title}</Typography>
            {hint ? <Typography variant="body2" color="text.secondary">{hint}</Typography> : null}
        </Box>
    );

    function openAssignment(aId: string) { setPlayerAssignmentId(aId); }
    // Helper to navigate AppShell to the Assignments page
    function goToAssignments() {
        try {
            window.dispatchEvent(new CustomEvent('appshell:navigate', { detail: { id: 's-assignments' } }));
        } catch { /* ignore */ }
    }
    // From dashboard tiles, jump to Assignments page and open the player
    function openFromDashboard(aId?: string, seedSql?: string) {
        if (aId) setPlayerAssignmentId(aId);
        else if (seedSql) { setSql(seedSql); setEditorOpen(true); }
        goToAssignments();
    }

    async function refreshSubmissions() {
        if (!user?.token) return;
        try {
            const res = await authFetch('/api/submissions');
            const data = await res.json();
            setSubmissions(Array.isArray(data) ? data : []);
        } catch { /* ignore */ }
    }

    return (
        <Box sx={{ display: 'grid', gap: 2 }}>
            {/* Dashboard */}
            {active === 's-dash' && (
                <PageCard>
                    {header('Dashboard', 'Overview of your assignments and recent activity.')}
                    <Box sx={{ display: 'grid', gap: 2, gridTemplateColumns: { md: 'repeat(3,1fr)', xs: '1fr' } }}>
                        <StatCard icon={AssignmentTurnedInIcon} label="Total assignments" value={assignments.length} tone="secondary" />
                        <StatCard icon={TimerIcon} label="Nearest due" value={nearestDueDays} postfix="days" tone={nearestDueTone} />
                        <StatCard icon={CheckCircleIcon} label="Finished" value={categorized.finished.length} />
                    </Box>

                    {categorized.overdue.length > 0 && (
                        <Card sx={{ mt: 2, borderColor: 'error.main' }}>
                            <CardHeader title={<Typography variant="subtitle1" sx={{ color: 'error.main' }}>Overdue</Typography>} subheader={<Typography variant="caption" color="text.secondary">Past the deadline and not finished.</Typography>} />
                            <CardContent>
                                <AssignmentsGrid openEditor={(seed, aId) => openFromDashboard(aId, seed)} assignments={categorized.overdue} submittedTitles={categorized.finishedTitles} inProgressTitles={categorized.inProgressTitles} />
                            </CardContent>
                        </Card>
                    )}

                    {categorized.inProgress.length > 0 && (
                        <Card sx={{ mt: 2 }}>
                            <CardHeader title={<Typography variant="subtitle1">In Progress</Typography>} subheader={<Typography variant="caption" color="text.secondary">Assignments you have started.</Typography>} />
                            <CardContent>
                                <AssignmentsGrid openEditor={(seed, aId) => openFromDashboard(aId, seed)} assignments={categorized.inProgress} submittedTitles={categorized.finishedTitles} inProgressTitles={categorized.inProgressTitles} />
                            </CardContent>
                        </Card>
                    )}

                    {categorized.pending.length > 0 && (
                        <Card sx={{ mt: 2 }}>
                            <CardHeader title={<Typography variant="subtitle1">Pending</Typography>} subheader={<Typography variant="caption" color="text.secondary">Not started yet.</Typography>} />
                            <CardContent>
                                <AssignmentsGrid openEditor={(seed, aId) => openFromDashboard(aId, seed)} assignments={categorized.pending} submittedTitles={categorized.finishedTitles} inProgressTitles={categorized.inProgressTitles} />
                            </CardContent>
                        </Card>
                    )}

                    {categorized.finished.length > 0 && (
                        <Card sx={{ mt: 2 }}>
                            <CardHeader title={<Typography variant="subtitle1">Finished</Typography>} subheader={<Typography variant="caption" color="text.secondary">Submitted or auto-graded.</Typography>} />
                            <CardContent>
                                <AssignmentsGrid openEditor={(seed, aId) => openFromDashboard(aId, seed)} assignments={categorized.finished} submittedTitles={categorized.finishedTitles} inProgressTitles={categorized.inProgressTitles} />
                            </CardContent>
                        </Card>
                    )}
                </PageCard>
            )}

            {/* Assignments */}
            {active === 's-assignments' && (
                <PageCard>
                    {header('Assignments', 'All assigned work for this course.')}
                    {playerAssignmentId ? (
                        <StudentAssignmentPlayer
                            assignmentId={playerAssignmentId}
                            onClose={() => { setPlayerAssignmentId(null); refreshSubmissions(); }}
                            onSubmitted={() => { refreshSubmissions(); }}
                        />
                    ) : (
                        <AssignmentsGrid openEditor={(seed, aId) => { if (aId) { openAssignment(aId); } else { setSql(seed || sql); setEditorOpen(true); } }} assignments={assignments} submittedTitles={submittedTitles} inProgressTitles={categorized.inProgressTitles} highlightOverdueBorder />
                    )}
                </PageCard>
            )}

            {/* Submissions */}
            {active === 's-submissions' && (
                <PageCard>
                    {header('Your Submissions', 'History of what you’ve submitted.')}
                    <StudentSubmissions rows={submissions} />
                </PageCard>
            )}

            {/* Profile */}
            {active === 's-profile' && (
                <PageCard>
                    {header('Profile & Preferences', 'Name, theme and editor preferences.')}
                    <StudentProfile />
                </PageCard>
            )}

            {/* Fallback */}
            {!['s-dash', 's-assignments', 's-submissions', 's-profile'].includes(active) && (
                <Typography variant="body2" color="text.secondary">Select an item from the sidebar.</Typography>
            )}

            {/* Editor dialog lives here so it can be opened from any page */}
            <EditorDialog
                open={editorOpen}
                setOpen={setEditorOpen}
                sql={sql}
                setSql={setSql}
                grading={grading}
                grade={grade}
                feedback={feedback}
                rubric={rubric}
                onRun={runGrade}
            />
        </Box>
    );
}
