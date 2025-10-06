import crypto from 'crypto';

// Lightweight HMAC-SHA256 JWT implementation (HS256) without external deps
// NOTE: For production WE rotate secrets.

const ALG = 'HS256';
const TYP = 'JWT';

function base64url(input: Buffer | string) {
    return Buffer.from(input).toString('base64').replace(/=/g, '').replace(/\+/g, '-').replace(/\//g, '_');
}

export type JwtRole = 'instructor' | 'student';
export interface JwtPayload {
    sub: string; // subject: instructor id for instructor tokens, roster id for student tokens
    email: string;
    name: string;
    role: JwtRole;
    // when role=student, include owning instructor id
    instructorId?: string;
    // when role=instructor, instructorId is implicitly sub
    iat: number;
    exp: number;
}

export function signJwt(payload: Omit<JwtPayload, 'iat' | 'exp'>, opts?: { expiresInSec?: number }) {
    const header = { alg: ALG, typ: TYP };
    const iat = Math.floor(Date.now() / 1000);
    const exp = iat + (opts?.expiresInSec || 60 * 60 * 8); // default 8h
    const full: JwtPayload = { ...payload, iat, exp };
    const secret = process.env.JWT_SECRET || 'dev-insecure-secret-change';
    const headB64 = base64url(JSON.stringify(header));
    const bodyB64 = base64url(JSON.stringify(full));
    const toSign = `${headB64}.${bodyB64}`;
    const sig = crypto.createHmac('sha256', secret).update(toSign).digest('base64').replace(/=/g, '').replace(/\+/g, '-').replace(/\//g, '_');
    return `${toSign}.${sig}`;
}

// Convenience for student tokens
export function signStudentJwt(args: { rosterId: string; instructorId: string; email: string; name: string }, opts?: { expiresInSec?: number }) {
    const { rosterId, instructorId, email, name } = args;
    return signJwt({ sub: rosterId, instructorId, email, name, role: 'student' }, opts);
}

export function verifyJwt(token: string): JwtPayload | null {
    try {
        const secret = process.env.JWT_SECRET || 'dev-insecure-secret-change';
        const [h, p, s] = token.split('.');
        if (!h || !p || !s) return null;
        const expected = crypto.createHmac('sha256', secret).update(`${h}.${p}`).digest('base64').replace(/=/g, '').replace(/\+/g, '-').replace(/\//g, '_');
        if (expected !== s) return null;
        const payload = JSON.parse(Buffer.from(p, 'base64').toString('utf8')) as JwtPayload;
        if (payload.exp && payload.exp < Math.floor(Date.now() / 1000)) return null;
        return payload;
    } catch {
        return null;
    }
}

export function parseAuthHeader(h?: string): string | null {
    if (!h) return null;
    const m = /^Bearer\s+(.+)$/.exec(h.trim());
    return m ? m[1] : null;
}
