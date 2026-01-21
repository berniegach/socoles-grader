import crypto from 'crypto';

//JWT - JSON Web Token
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
    // when role=student, include course context (optional; falls back to instructor default)
    courseId?: string;
    // when role=instructor, instructorId is implicitly sub
    iat: number; // issued at timestamp
    exp: number; // expiration timestamp
}
/**
 * Sign a JWT token with the given payload and options.
 * 1. Proves who the user is.
 * 2. Prevents the data in the token from being changed (the signature will no longer match).
 * 3. Puts an expiry time on the token so it stops working after a while.
 * 4. Lets the server verify requests without storing sessions in the database.
 *
 * In short: it creates a tamper-proof, time-limited “login ticket” that the client can send with each request.
 */
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
/**
 * Create a JWT for a student.
 * 1. Students belong to an instructor (and usually a course), so we include that info in the token.
 * 2. The roster id goes in `sub`, so API routes can identify the student.
 *
 * In short: it makes a student “login ticket” with the extra context the app needs.
 */
export function signStudentJwt(
    args: { rosterId: string; instructorId: string; courseId?: string; email: string; name: string },
    opts?: { expiresInSec?: number }
) {
    const { rosterId, instructorId, courseId, email, name } = args;
    return signJwt({ sub: rosterId, instructorId, courseId, email, name, role: 'student' }, opts);
}

/**
* Verify and decode a JWT.
* 1. Checks the signature so we know the token was created by us.
* 2. Checks the expiry time so old tokens stop working.
*
* Returns the payload when valid, otherwise returns null.
*/
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
/**
* Read an Authorization header and extract the Bearer token.
* 1. Requests send tokens as `Authorization: Bearer <token>`.
* 2. This helper returns just the token string (or null if missing/invalid).
*/
export function parseAuthHeader(h?: string): string | null {

    if (!h) return null;
    const m = /^Bearer\s+(.+)$/.exec(h.trim());
    return m ? m[1] : null;
}
