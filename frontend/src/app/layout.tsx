import type { Metadata } from 'next';
import { CssBaseline, ThemeProvider } from '@mui/material';
import theme from '@/theme/theme';
import 'katex/dist/katex.min.css';
import { AppRouterCacheProvider } from '@mui/material-nextjs/v14-appRouter';
import { AuthBootClient } from './AuthBootClient';

export const metadata: Metadata = {
  title: 'SOCOLES',
  description: 'An SQL grader with for students and instructors',
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <body>
        <AppRouterCacheProvider options={{ key: 'mui' }}>
          <ThemeProvider theme={theme}>
            <CssBaseline />
            <AuthBootClient>{children}</AuthBootClient>
          </ThemeProvider>
        </AppRouterCacheProvider>
      </body>
    </html>
  );
}
