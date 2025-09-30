'use client';
import { useMemo, useState, useEffect } from 'react';
import { Box, Drawer, Toolbar, AppBar, Typography, IconButton, List, ListItemButton, ListItemIcon, ListItemText, Avatar, Divider } from '@mui/material';
import DashboardIcon from '@mui/icons-material/DashboardRounded';
import MenuBookIcon from '@mui/icons-material/MenuBook';
import DescriptionIcon from '@mui/icons-material/DescriptionOutlined';
import GroupIcon from '@mui/icons-material/GroupOutlined';
import AssignmentIcon from '@mui/icons-material/AssignmentTurnedInOutlined';
import CloudUploadIcon from '@mui/icons-material/CloudUploadOutlined';
import SettingsIcon from '@mui/icons-material/SettingsOutlined';
import StorageIcon from '@mui/icons-material/StorageOutlined';
import LogoutIcon from '@mui/icons-material/Logout';
// import SentimentSatisfiedAltIcon from '@mui/icons-material/SentimentSatisfiedAlt';

import { useAuth } from '@/features/auth/AuthProvider';
import StudentArea from '@/features/student/StudentArea';
import InstructorArea from '@/features/instructor/InstructorArea';
import type { Role } from '@/lib/types';

const drawerWidth = 260;
type Item = { id: string; label: string; icon: React.ElementType };

export default function AppShell() {
    const { user, setUser } = useAuth();
    const role = (user?.role || 'student') as Role;

    const items: Item[] = useMemo(
        () => role === 'student'
            ? [
                { id: 's-dash', label: 'Dashboard', icon: DashboardIcon },
                { id: 's-assignments', label: 'Assignments', icon: MenuBookIcon },
                { id: 's-submissions', label: 'Submissions', icon: DescriptionIcon },
                { id: 's-profile', label: 'Profile', icon: GroupIcon },
            ]
            : [
                { id: 'i-dash', label: 'Dashboard', icon: DashboardIcon },
                { id: 'i-questions', label: 'Questions', icon: AssignmentIcon },
                { id: 'i-assignments', label: 'Assignments', icon: MenuBookIcon },
                { id: 'i-datasets', label: 'Datasets', icon: StorageIcon },
                { id: 'i-batch', label: 'Batch Grader', icon: CloudUploadIcon },
                { id: 'i-submissions', label: 'Submissions', icon: DescriptionIcon },
                { id: 'i-class', label: 'Class', icon: GroupIcon },
                { id: 'i-settings', label: 'Settings', icon: SettingsIcon },
            ],
        [role]
    );

    const [active, setActive] = useState(items[0].id);

    // Allow children to request navigation via a custom event
    useEffect(() => {
        function onNavigate(e: Event) {
            const detail = (e as CustomEvent).detail as { id?: string } | undefined;
            if (detail?.id) {
                setActive(detail.id);
                sessionStorage.setItem('appshell.active', detail.id);
            }
        }
        window.addEventListener('appshell:navigate', onNavigate as EventListener);
        // Restore last active on mount
        const stored = sessionStorage.getItem('appshell.active');
        if (stored) setActive(stored);
        return () => window.removeEventListener('appshell:navigate', onNavigate as EventListener);
    }, []);

    // Compute display name: for students prefer instructor-managed full name from stored session
    const [displayName, setDisplayName] = useState<string | null>(null);
    useEffect(() => {
        if (role === 'student') {
            try {
                const raw = localStorage.getItem('sqlgrader.student');
                if (raw) {
                    const parsed = JSON.parse(raw);
                    if (parsed?.name) setDisplayName(parsed.name as string);
                }
            } catch { /* ignore */ }
        } else {
            setDisplayName(null);
        }
    }, [role]);

    return (
        <Box sx={{ display: 'flex', minHeight: '100vh' }}>
            <AppBar position="fixed" sx={{ zIndex: (t) => t.zIndex.drawer + 1 }}>
                <Toolbar sx={{ gap: 2 }}>
                    <Box component="img" src="/icons/socoles-logo.png" alt="SOCOLES" sx={{ height: 32, width: 'auto', display: 'block' }} />
                    <Typography variant="h6" sx={{ flexGrow: 1 }}>
                        Welcome back, {displayName || user?.name?.split('@')[0] || 'friend'}!
                    </Typography>
                    <IconButton color="inherit" onClick={() => { try { localStorage.removeItem('sqlgrader.instructor'); localStorage.removeItem('sqlgrader.student'); } catch { } setUser(null); }} title="Sign out">
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
                    <Box component="img" src="/icons/socoles-logo.png" alt="SOCOLES Logo" sx={{ height: 40, width: 'auto', display: 'block' }} />
                    <Box>
                        <Typography fontWeight={600}>SOCOLES</Typography>
                        <Typography variant="caption" color="text.secondary">Making grading easier</Typography>
                    </Box>
                </Box>
                <Divider />
                <List>
                    {items.map((it) => (
                        <ListItemButton key={it.id} selected={active === it.id} onClick={() => { setActive(it.id); sessionStorage.setItem('appshell.active', it.id); }}>
                            <ListItemIcon>
                                <it.icon color="secondary" />
                            </ListItemIcon>
                            <ListItemText primary={it.label} />
                        </ListItemButton>
                    ))}
                </List>
                <Box sx={{ mt: 'auto', p: 2, display: 'flex', alignItems: 'center', gap: 1.5 }}>
                    <Avatar
                        src={role === 'student' ? '/icons/reading-book.png' : '/icons/teacher.png'}
                        alt={role === 'student' ? 'Student' : 'Instructor'}
                        sx={{ width: 36, height: 36, bgcolor: 'secondary.main' }}
                    />
                    <Box>
                        <Typography variant="body2" noWrap sx={{ maxWidth: 140 }}>{user?.name}</Typography>
                        <Typography variant="caption" color="text.secondary">{role}</Typography>
                    </Box>
                </Box>
            </Drawer>

            <Box component="main" sx={{ flexGrow: 1 }}>
                <Toolbar />
                <Box sx={{ maxWidth: 1040, mx: 'auto', px: 3, pt: 2, pb: 5 }}>
                    {role === 'student' ? <StudentArea active={active} /> : <InstructorArea active={active} />}
                </Box>
            </Box>
        </Box>
    );
}
