'use client';
import { useMemo, useState, useEffect, useRef } from 'react';
import { Box, Drawer, Toolbar, AppBar, Typography, IconButton, List, ListItemButton, ListItemIcon, ListItemText, Avatar, Divider, ListSubheader, Chip } from '@mui/material';
import DashboardIcon from '@mui/icons-material/DashboardRounded';
import MenuBookIcon from '@mui/icons-material/MenuBook';
import DescriptionIcon from '@mui/icons-material/DescriptionOutlined';
import GroupIcon from '@mui/icons-material/GroupOutlined';
import AssignmentIcon from '@mui/icons-material/AssignmentTurnedInOutlined';
import CloudUploadIcon from '@mui/icons-material/CloudUploadOutlined';
import SettingsIcon from '@mui/icons-material/SettingsOutlined';
import RateReviewIcon from '@mui/icons-material/RateReviewOutlined';
import StorageIcon from '@mui/icons-material/StorageOutlined';
import LogoutIcon from '@mui/icons-material/Logout';

import StudentArea from '@/features/student/StudentArea';
import InstructorArea from '@/features/instructor/InstructorArea';
import InstructorReviewRequests from '@/features/instructor/InstructorReviewRequests';
import ClassRoster from '@/features/instructor/ClassRoster';
import type { Role } from '@/lib/types';
import { useAuth } from '@/features/auth/AuthProvider';


const drawerWidth = 260;
type Item = { id: string; label: string; icon: React.ElementType };

export default function AppShell() {
    const { user, setUser, authFetch } = useAuth();
    const role = (user?.role || 'student') as Role;

    const studentItems: Item[] = useMemo(
        () => role === 'student'
            ? [
                { id: 's-dash', label: 'Dashboard', icon: DashboardIcon },
                { id: 's-assignments', label: 'Assignments', icon: MenuBookIcon },
                { id: 's-submissions', label: 'Submissions', icon: DescriptionIcon },
                { id: 's-profile', label: 'Profile', icon: GroupIcon },
            ]
            : [],
        [role]
    );
    const taItems: Item[] = useMemo(
        () => (role === 'student' && user?.evaluator)
            ? [
                { id: 'i-reviews', label: 'Reviews', icon: RateReviewIcon },
                { id: 'i-submissions', label: 'Submissions', icon: DescriptionIcon },
                { id: 'i-class', label: 'Roster', icon: GroupIcon },
            ]
            : [],
        [role, user?.evaluator]
    );
    const instructorItems: Item[] = useMemo(
        () => role === 'instructor'
            ? [
                { id: 'i-dash', label: 'Dashboard', icon: DashboardIcon },
                { id: 'i-questions', label: 'Questions', icon: AssignmentIcon },
                { id: 'i-assignments', label: 'Assignments', icon: MenuBookIcon },
                { id: 'i-datasets', label: 'Datasets', icon: StorageIcon },
                { id: 'i-batch', label: 'Batch Grader', icon: CloudUploadIcon },
                { id: 'i-submissions', label: 'Submissions', icon: DescriptionIcon },
                { id: 'i-reviews', label: 'Reviews', icon: RateReviewIcon },
                { id: 'i-class', label: 'Class', icon: GroupIcon },
                { id: 'i-settings', label: 'Settings', icon: SettingsIcon },
            ]
            : [],
        [role]
    );
    const combinedItems: Item[] = useMemo(
        () => role === 'student' ? [...studentItems, ...taItems] : instructorItems,
        [role, studentItems, taItems, instructorItems]
    );

    const [active, setActive] = useState(combinedItems[0]?.id || (role === 'student' ? 's-dash' : 'i-dash'));
    const [pendingCount, setPendingCount] = useState<number>(0);
    const pollRef = useRef<any>(null);

    // Poll pending review requests count for instructors and TAs
    useEffect(() => {
        const eligible = role === 'instructor' || (role === 'student' && user?.evaluator);
        if (!eligible) { setPendingCount(0); if (pollRef.current) { clearInterval(pollRef.current); pollRef.current = null; } return; }
        async function fetchCount() {
            try {
                const res = await authFetch('/api/review-requests?status=Pending');
                if (res.ok) {
                    const arr = await res.json();
                    setPendingCount(Array.isArray(arr) ? arr.length : 0);
                }
            } catch { /* ignore */ }
        }
        // initial load and start polling
        void fetchCount();
        if (!pollRef.current) pollRef.current = setInterval(fetchCount, 8000);
        return () => { if (pollRef.current) { clearInterval(pollRef.current); pollRef.current = null; } };
    }, [role, user?.evaluator, authFetch]);

    // Allow children to request navigation via a custom event
    useEffect(() => {
        function onNavigate(e: Event) {
            const detail = (e as CustomEvent).detail as { id?: string; submissionId?: string; questionId?: string; assignmentId?: string } | undefined;
            if (detail?.id) {
                setActive(detail.id);
                sessionStorage.setItem('appshell.active', detail.id);
                const { submissionId, questionId, assignmentId } = detail;
                if (submissionId || questionId || assignmentId) {
                    sessionStorage.setItem('appshell.deepLink', JSON.stringify({ submissionId, questionId, assignmentId, ts: Date.now() }));
                }
            }
        }
        window.addEventListener('appshell:navigate', onNavigate as EventListener);
        // Restore last active on mount
        const stored = sessionStorage.getItem('appshell.active');
        if (stored) setActive(stored);
        return () => window.removeEventListener('appshell:navigate', onNavigate as EventListener);
    }, []);

    // Get course name for app bar (for instructors)
    const [courseName, setCourseName] = useState<string>('');
    useEffect(() => {
        (async () => {
            try {
                let res;
                if (role === 'instructor') {
                    res = await authFetch('/api/instructor/settings', { method: 'GET' });
                } else {
                    res = await authFetch('/api/student/course', { method: 'GET' });
                }
                if (res.ok) {
                    const data = await res.json();
                    setCourseName(data.course_name || '');
                } else {
                    setCourseName('');
                }
            } catch {
                setCourseName('');
            }
        })();
    }, [role, authFetch]);

    const handleSignOut = async () => {
        try {
            // Clear any local app auth caches first
            localStorage.removeItem('sqlgrader.instructor');
            localStorage.removeItem('sqlgrader.student');
            sessionStorage.removeItem('appshell.active');
            sessionStorage.removeItem('appshell.deepLink');
        } catch { /* ignore */ }
        setUser(null);
        // Perform full logout (local + IdP). Route will handle local signout first, then IdP end-session.
        if (typeof window !== 'undefined') {
            window.location.href = '/api/auth/idp-logout?redirect=/';
            return;
        }
    };

    return (
        <Box sx={{ display: 'flex', minHeight: '100vh' }}>
            <AppBar position="fixed" sx={{ zIndex: (t) => t.zIndex.drawer + 1, background: 'linear-gradient(90deg,#ff66c4,#ffde59)', boxShadow: '0 2px 6px rgba(0,0,0,0.25)' }}>
                <Toolbar sx={{ gap: 2, minHeight: 60 }}>
                    <Box component="img" src="/icons/student.gif" alt="Student" sx={{ height: 40, width: 'auto', display: 'block', borderRadius: 1, boxShadow: '0 0 0 2px rgba(255,255,255,0.25)' }} />
                    <Typography variant="h6" sx={{ flexGrow: 1, fontWeight: 600, letterSpacing: 0.25 }}>
                        {courseName || 'Course'}
                    </Typography>
                    <IconButton color="inherit" onClick={handleSignOut} title="Sign out" sx={{ bgcolor: 'rgba(0,0,0,0.15)', '&:hover': { bgcolor: 'rgba(0,0,0,0.25)' } }}>
                        <LogoutIcon />
                    </IconButton>
                </Toolbar>
            </AppBar>

            <Drawer variant="permanent" sx={{
                width: drawerWidth, flexShrink: 0,
                [`& .MuiDrawer-paper`]: { width: drawerWidth, boxSizing: 'border-box' }
            }}>
                <Toolbar />
                <Box sx={{ p: 2, display: 'flex', alignItems: 'center', gap: 1.5 }}>
                    <Box component="img" src="/icons/socoles-logo.svg" alt="SOCOLES Logo" sx={{ height: 40, width: 'auto', display: 'block' }} />
                    <Box>
                        <Typography fontWeight={600}>SOCOLES</Typography>
                        <Typography variant="caption" color="text.secondary">Making grading easier</Typography>
                    </Box>
                </Box>
                <Divider />
                <List sx={{ mt: 1 }}>
                    {(role === 'student' ? studentItems : instructorItems).map((it) => {
                        const isActive = active === it.id;
                        return (
                            <ListItemButton
                                key={it.id}
                                aria-current={isActive ? 'page' : undefined}
                                onClick={() => { setActive(it.id); sessionStorage.setItem('appshell.active', it.id); }}
                                sx={(theme) => ({
                                    position: 'relative',
                                    borderRadius: 1,
                                    mx: 1,
                                    my: 0.25,
                                    pl: 1.5,
                                    alignItems: 'center',
                                    gap: 0.5,
                                    overflow: 'hidden',
                                    transition: 'background .2s, transform .15s',
                                    ...(isActive && {
                                        background: 'linear-gradient(90deg, rgba(255,102,196,0.16), rgba(255,222,89,0.16))'
                                    }),
                                    '&:hover': {
                                        background: isActive
                                            ? 'linear-gradient(90deg, rgba(255,102,196,0.25), rgba(255,222,89,0.25))'
                                            : 'rgba(255,255,255,0.06)'
                                    },
                                    '&:before': isActive ? {
                                        content: '""',
                                        position: 'absolute',
                                        left: 0,
                                        top: 4,
                                        bottom: 4,
                                        width: 4,
                                        borderRadius: 2,
                                        background: 'linear-gradient(180deg,#ff66c4,#ffde59)'
                                    } : undefined
                                })}
                            >
                                <ListItemIcon sx={{ minWidth: 40, color: isActive ? 'secondary.main' : 'text.secondary' }}>
                                    <it.icon fontSize='small' />
                                </ListItemIcon>
                                <ListItemText primaryTypographyProps={{ fontWeight: isActive ? 600 : 500 }} primary={it.label} />
                                {it.id === 'i-reviews' && pendingCount > 0 && (
                                    <Box component="span" sx={{ ml: 'auto', mr: .5 }}>
                                        <Chip size='small' color='warning' label={pendingCount > 99 ? '99+' : String(pendingCount)} sx={{ height: 20 }} />
                                    </Box>
                                )}
                            </ListItemButton>
                        );
                    })}
                    {role === 'student' && taItems.length > 0 && (
                        <ListSubheader disableSticky sx={{ mt: 1, mb: 0.5, mx: 1.5, px: 0, fontSize: 12, fontWeight: 700, textTransform: 'uppercase', color: 'text.secondary' }}>
                            TA tools
                        </ListSubheader>
                    )}
                    {role === 'student' && taItems.map((it) => {
                        const isActive = active === it.id;
                        return (
                            <ListItemButton
                                key={it.id}
                                aria-current={isActive ? 'page' : undefined}
                                onClick={() => { setActive(it.id); sessionStorage.setItem('appshell.active', it.id); }}
                                sx={{
                                    position: 'relative',
                                    borderRadius: 1,
                                    mx: 1,
                                    my: 0.25,
                                    pl: 1.5,
                                    alignItems: 'center',
                                    gap: 0.5,
                                    overflow: 'hidden',
                                    transition: 'background .2s, transform .15s',
                                    ...(isActive && { background: 'rgba(255,255,255,0.08)' }),
                                    '&:hover': { background: isActive ? 'rgba(255,255,255,0.14)' : 'rgba(255,255,255,0.06)' },
                                    '&:before': isActive ? {
                                        content: '""',
                                        position: 'absolute',
                                        left: 0,
                                        top: 4,
                                        bottom: 4,
                                        width: 4,
                                        borderRadius: 2,
                                        background: 'linear-gradient(180deg,#4e54c8,#8f94fb)'
                                    } : undefined
                                }}
                            >
                                <ListItemIcon sx={{ minWidth: 40, color: isActive ? 'secondary.main' : 'text.secondary' }}>
                                    <it.icon fontSize='small' />
                                </ListItemIcon>
                                <ListItemText primaryTypographyProps={{ fontWeight: isActive ? 600 : 500 }} primary={it.label} />
                                {it.id === 'i-reviews' && pendingCount > 0 && (
                                    <Box component="span" sx={{ ml: 'auto', mr: .5 }}>
                                        <Chip size='small' color='warning' label={pendingCount > 99 ? '99+' : String(pendingCount)} sx={{ height: 20 }} />
                                    </Box>
                                )}
                            </ListItemButton>
                        );
                    })}
                </List>
                <Box sx={{ mt: 'auto', p: 2, display: 'flex', alignItems: 'center', gap: 1.25, opacity: 0.9 }}>
                    <Avatar src={role === 'student' ? '/icons/reading-book.png' : '/icons/teacher.png'} alt={role === 'student' ? 'Student' : 'Instructor'} sx={{ width: 36, height: 36, bgcolor: 'secondary.main' }} />
                    <Box sx={{ minWidth: 0 }}>
                        <Typography variant="body2" noWrap sx={{ maxWidth: 140, fontWeight: 600 }}>{user?.name}</Typography>
                        <Typography variant="caption" color="text.secondary">{role === 'student' ? (user?.evaluator ? 'student • TA' : 'student') : 'instructor'}</Typography>
                    </Box>
                </Box>
            </Drawer>

            <Box component="main" sx={{ flexGrow: 1 }}>
                <Toolbar />
                <Box sx={{ maxWidth: 1040, mx: 'auto', px: 3, pt: 2, pb: 5 }}>
                    {role === 'student'
                        ? (user?.evaluator && (active === 'i-reviews' || active === 'i-class' || active === 'i-submissions')
                            ? (active === 'i-reviews' ? <InstructorReviewRequests /> : active === 'i-class' ? <ClassRoster /> : <InstructorArea active="i-submissions" />)
                            : <StudentArea active={active} />)
                        : <InstructorArea active={active} />}
                </Box>
            </Box>
        </Box>
    );
}
