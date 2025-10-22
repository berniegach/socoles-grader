import NextAuth from 'next-auth'
import Keycloak from 'next-auth/providers/keycloak'
import type { JWT } from 'next-auth/jwt'
import type { Account, Session } from 'next-auth'

const config = {
    providers: [
        (() => {
            const internalIssuer = process.env.KEYCLOAK_ISSUER
            const publicIssuer = process.env.KEYCLOAK_ISSUER_PUBLIC || internalIssuer
            const trim = (s?: string) => (s ? s.replace(/\/$/, '') : '')
            const p = Keycloak({
                clientId: process.env.KEYCLOAK_CLIENT_ID!,
                clientSecret: process.env.KEYCLOAK_CLIENT_SECRET || '',
                // Expect the issuer reported by Keycloak (iss) to match what the browser used
                // Use publicIssuer when available to avoid issuer mismatches between dev (localhost)
                // and Docker (where server talks to keycloak service).
                issuer: publicIssuer || internalIssuer,
            })
            // Split endpoints: browser hits public host; server uses internal host
            if (publicIssuer) (p as any).authorization = `${trim(publicIssuer)}/protocol/openid-connect/auth`
            if (internalIssuer) {
                ; (p as any).token = `${trim(internalIssuer)}/protocol/openid-connect/token`
                    ; (p as any).userinfo = `${trim(internalIssuer)}/protocol/openid-connect/userinfo`
            }
            return p
        })(),
    ],
    session: { strategy: 'jwt' as const },
    callbacks: {
        async jwt({ token, account, profile }: { token: JWT; account?: Account | null; profile?: any }) {
            if (account) {
                // Persist tokens from Keycloak
                ; (token as any).accessToken = (account as any).access_token as string | undefined
                    ; (token as any).refreshToken = (account as any).refresh_token as string | undefined
                    ; (token as any).idToken = (account as any).id_token as string | undefined
            }
            const roles: string[] = (profile as any)?.realm_access?.roles || []
            if (roles.includes('instructor')) (token as any).role = 'instructor'
            if (roles.includes('student')) (token as any).role = (token as any).role || 'student'
            return token
        },
        async session({ session, token }: { session: Session; token: JWT }) {
            ; (session as any).accessToken = (token as any).accessToken
                ; (session as any).idToken = (token as any).idToken
                ; (session as any).refreshToken = (token as any).refreshToken
                ; (session.user as any).role = (token as any).role || (session.user as any)?.role || 'student'
            return session
        },
    },
}

export const { handlers, auth, signIn, signOut } = NextAuth(config)
