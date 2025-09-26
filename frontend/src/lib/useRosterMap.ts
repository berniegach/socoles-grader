import { useEffect, useState } from 'react';
import { useAuth } from '@/features/auth/AuthProvider';
import type { RosterEntry } from '@/lib/types';

/**
 * Build an email->full name map from the instructor's roster.
 * Falls back to the email when name is missing or when not found.
 */
export function useRosterMap() {
    const { user, authFetch } = useAuth();
    const [map, setMap] = useState<Record<string, string>>({});

    useEffect(() => {
        if (!user?.token) return;
        let cancelled = false;
        (async () => {
            try {
                const res = await authFetch('/api/roster');
                if (!res.ok) return;
                const rows = (await res.json()) as RosterEntry[];
                const m: Record<string, string> = {};
                (Array.isArray(rows) ? rows : []).forEach((r) => {
                    if (r?.email) m[r.email] = (r?.name || '').trim() || r.email;
                });
                if (!cancelled) setMap(m);
            } catch {
                // ignore
            }
        })();
        return () => { cancelled = true; };
    }, [user?.token, authFetch]);

    return map;
}
