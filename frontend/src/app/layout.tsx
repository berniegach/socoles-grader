import type { Metadata } from 'next';
import { CssBaseline, ThemeProvider } from '@mui/material';
import theme from '@/theme/theme';
import 'katex/dist/katex.min.css';
import { AppRouterCacheProvider } from '@mui/material-nextjs/v14-appRouter';
import { AuthBootClient } from './AuthBootClient';

export const metadata: Metadata = {
  title: 'SOCOLES',
  description: 'An SQL grader for students and instructors',
  icons: {
    icon: [
      { url: '/icons/socoles-logo.svg', type: 'image/svg+xml' },
      { url: '/icons/socoles-logo.ico' }
    ],
    shortcut: ['/icons/socoles-logo.ico']
  }
};

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="en">
      <head>
        <link rel="icon" type="image/svg+xml" href="/icons/socoles-logo.svg" />
        <link rel="shortcut icon" href="/icons/socoles-logo.ico" />
      </head>
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
