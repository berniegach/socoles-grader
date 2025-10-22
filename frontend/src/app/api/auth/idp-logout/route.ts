import { NextRequest, NextResponse } from 'next/server';
import { signOut as serverSignOut } from '@/auth';

// GET /api/auth/idp-logout?redirect=/
// Clears local session, then redirects the browser to Keycloak's logout page.
export async function GET(req: NextRequest) {
    try {
        const url = new URL(req.url);
        const redirectParam = url.searchParams.get('redirect') || '/';

        // Compute final app redirect (where the user lands after IdP logout)
        const baseRaw = process.env.KEYCLOAK_LOGOUT_REDIRECT_URI || process.env.NEXTAUTH_URL || `${url.protocol}//${url.host}`;
        const base = baseRaw.replace(/\/$/, '');
        const postLogout = redirectParam.startsWith('/') ? `${base}${redirectParam}` : `${base}/`;

        // First clear the local NextAuth session (no redirect from here)
        await serverSignOut({ redirect: false });

        // Then, redirect user to Keycloak end-session endpoint
        const issuer = process.env.KEYCLOAK_ISSUER_PUBLIC || process.env.KEYCLOAK_ISSUER;
        if (!issuer) return NextResponse.redirect(postLogout);

        const logoutUrl = new URL(issuer.replace(/\/$/, '') + '/protocol/openid-connect/logout');
        const clientId = process.env.KEYCLOAK_CLIENT_ID;
        if (clientId) logoutUrl.searchParams.set('client_id', clientId);
        logoutUrl.searchParams.set('post_logout_redirect_uri', postLogout);

        return NextResponse.redirect(logoutUrl.toString());
    } catch {
        // On any failure, just go back to home
        const url = new URL(req.url);
        const fallback = new URL('/', `${url.protocol}//${url.host}`).toString();
        return NextResponse.redirect(fallback);
    }
}
