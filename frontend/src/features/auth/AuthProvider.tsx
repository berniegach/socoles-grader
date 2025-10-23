'use client';
import { createContext, useContext } from 'react';
import type { Role } from '@/lib/types';

export type AuthUser = {
    name: string;
    email?: string;
    role: Role;
    token?: string;
    instructorId?: string;
    evaluator?: boolean;
};
export type AuthCtx = {
    user: AuthUser | null;
    setUser: (u: AuthUser | null) => void;
    authFetch: (input: RequestInfo | URL, init?: RequestInit) => Promise<Response>;
};

const Ctx = createContext<AuthCtx | undefined>(undefined);

export function AuthProvider({ value, children }: { value: AuthCtx; children: React.ReactNode }) {
    return <Ctx.Provider value={value}>{children}</Ctx.Provider>;
}

export function useAuth() {
    const ctx = useContext(Ctx);
    if (!ctx) throw new Error('useAuth must be used within AuthProvider');
    return ctx;
}
