import KeycloakProvider from 'next-auth/providers/keycloak';

export const authOptions = {
    providers: [
        KeycloakProvider({
            clientId: process.env.KEYCLOAK_CLIENT_ID!,
            clientSecret: process.env.KEYCLOAK_CLIENT_SECRET || '',
            issuer: process.env.KEYCLOAK_ISSUER!,
        })
    ],
    session: { strategy: 'jwt' as const },
    callbacks: {
        async jwt({ token, account, profile }: any) {
            if (account) {
                token.accessToken = account.access_token;
                token.refreshToken = account.refresh_token;
                token.idToken = account.id_token;
            }
            // Optional: infer role hints if Keycloak realm roles are present
            const roles: string[] = (profile?.realm_access?.roles as string[]) || [];
            if (roles.includes('instructor')) token.role = 'instructor';
            if (roles.includes('student')) token.role = token.role || 'student';
            return token;
        },
        async session({ session, token }: any) {
            (session as any).accessToken = token.accessToken;
            (session as any).idToken = token.idToken;
            (session as any).refreshToken = token.refreshToken;
            (session.user as any).role = token.role || (session.user?.role ?? 'student');
            return session;
        }
    }
};
