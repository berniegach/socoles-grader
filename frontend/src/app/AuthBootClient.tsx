"use client";
import React, { useState, useEffect, useCallback } from 'react';
import { AuthProvider } from '@/features/auth/AuthProvider';

export function AuthBootClient({ children }: { children: React.ReactNode }) {
    const [user, setUser] = useState<{ name: string; email?: string; role: 'instructor' | 'student'; token?: string; instructorId?: string; evaluator?: boolean } | null>(() => {
        // Synchronous restore so first render already has token
        if (typeof window === 'undefined') return null;
        try {
            const raw = localStorage.getItem('sqlgrader.instructor');
            if (raw) {
                const parsed = JSON.parse(raw);
                if (parsed?.token && parsed?.email) {
                    return { name: parsed.name || parsed.email, email: parsed.email, role: 'instructor', token: parsed.token, instructorId: parsed.id };
                }
            }
            const sraw = localStorage.getItem('sqlgrader.student');
            if (sraw) {
                const parsed = JSON.parse(sraw);
                if (parsed?.token && parsed?.email) {
                    return { name: parsed.name || parsed.email, email: parsed.email, role: 'student', token: parsed.token, instructorId: parsed.instructorId };
                }
            }
        } catch { }
        return null;
    });

    // logout helper
    const logout = useCallback(() => {
        try { localStorage.removeItem('sqlgrader.instructor'); localStorage.removeItem('sqlgrader.student'); } catch { }
        setUser(null);
    }, []);

    useEffect(() => {
        // Validate existing token (lightweight) by decoding exp, then optional roundtrip to /api/auth
        if (!user?.token) return;
        let expired = false;
        try {
            const parts = user.token.split('.');
            if (parts.length === 3) {
                const payload = JSON.parse(atob(parts[1]));
                if (payload?.exp && payload.exp < Math.floor(Date.now() / 1000)) expired = true;
            }
        } catch { }
        if (expired) {
            console.warn('[AuthBootClient] token expired, clearing');
            logout();
            return;
        }
        (async () => {
            try {
                const ep = user.role === 'instructor' ? '/api/auth' : '/api/student/login';
                // For student, we don't actually want to login with password; instead, ping a student-protected endpoint
                const url = user.role === 'instructor' ? ep : '/api/assignments';
                const res = await fetch(url, { headers: { Authorization: `Bearer ${user.token}` } });
                if (!res.ok) {
                    console.warn('[AuthBootClient] token rejected, clearing');
                    logout();
                }
                // For students, also fetch self to get evaluator flag and augment auth context
                if (user.role === 'student') {
                    try {
                        const me = await fetch('/api/student/me', { headers: { Authorization: `Bearer ${user.token}` } });
                        if (me.ok) {
                            const info = await me.json();
                            if (info && typeof info.evaluator === 'boolean') {
                                setUser(prev => prev ? { ...prev, evaluator: info.evaluator } : prev);
                            }
                        }
                    } catch { /* ignore */ }
                }
            } catch {
                // network errors ignored for now
            }
        })();
    }, [user?.token, logout]);

    const authFetch = useCallback((input: RequestInfo | URL, init?: RequestInit) => {
        const headers = new Headers(init?.headers || {});
        if (user?.token && !headers.has('Authorization')) headers.set('Authorization', `Bearer ${user.token}`);
        const urlStr = typeof input === 'string' ? input : (input instanceof URL ? input.toString() : 'Request');
        const authHeader = headers.get('Authorization');
        console.log('[authFetch] ->', urlStr, 'hdr?', authHeader ? (authHeader.startsWith('Bearer ') ? 'Bearer ' + authHeader.substring(7, 15) + '...' : authHeader.substring(0, 8) + '...') : 'none');
        return fetch(input, { ...init, headers });
    }, [user]);

    useEffect(() => {
        console.log('[AuthBootClient] user state changed', user?.name, 'token?', !!user?.token);
        if (typeof window !== 'undefined') {
            (window as unknown as { __AUTH?: unknown }).__AUTH = { user, setUser };
        }
    }, [user]);

    return <AuthProvider value={{ user, setUser, authFetch }}>{children}</AuthProvider>;
}
