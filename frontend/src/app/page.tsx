"use client";
import React from 'react';
import AppShell from '@/components/AppShell';
import { useAuth } from '@/features/auth/AuthProvider';
import { Box, Card, CardContent, CardActionArea, Typography } from '@mui/material';
import Grid from '@mui/material/Grid';
import Avatar from '@mui/material/Avatar';
import Link from 'next/link';
import Logo from '@/components/Logo';

export default function HomePage() {
  const { user } = useAuth();
  if (user) return <AppShell />;

  return (
    <Box
      sx={{
        minHeight: '100vh',
        display: 'grid',
        placeItems: 'center',
        p: 3,
        // Background: theme.background.default with a subtle radial accent like the homepage concept
        background: (t) => `radial-gradient(900px 480px at 80% -10%, rgba(29, 78, 216, 0.08), transparent 60%), ${t.palette.background.default}`,
      }}
    >
      <style>{`
        @keyframes borderGradientMove {
          0% { background-position: 0% 50%; }
          50% { background-position: 100% 50%; }
          100% { background-position: 0% 50%; }
        }
      `}</style>

      <Box sx={{ width: '100%', maxWidth: 720 }}>
        <Box sx={{ display: 'grid', placeItems: 'center', mb: 3 }}>
          <Logo height={56} variant="default" />
          <Typography variant="h5" sx={{ fontWeight: 700, mt: 1, color: '#191654' }}>SOCOLES</Typography>
          <Typography variant="body2" color="text.secondary">Choose your role to sign in</Typography>
        </Box>

        <Grid container spacing={2}>
          {/* Student Card (primary palette) */}
          <Grid size={{ xs: 12, md: 6 }}>
            <Link href={{ pathname: '/login/student' }} legacyBehavior passHref>
              <a style={{ textDecoration: 'none' }}>
                <Card
                  variant="outlined"
                  sx={{
                    borderRadius: '6px',
                    overflow: 'hidden',
                    borderColor: 'rgba(29, 78, 216, 0.10)',
                    boxShadow: '0 6px 20px rgba(29, 78, 216, 0.08)',
                    transition: 'transform 120ms ease, box-shadow 120ms ease',
                    '&:hover': {
                      transform: 'translateY(-2px)',
                      boxShadow: '0 10px 26px rgba(29, 78, 216, 0.12)'
                    }
                  }}
                >
                  <CardActionArea>
                    {/* Header bar to mirror homepage style */}
                    <Box
                      sx={{
                        background: 'linear-gradient(90deg,#191654,#43C6AC)',
                        color: 'primary.contrastText',
                        textAlign: 'center',
                        fontWeight: 600,
                        fontSize: 14,
                        py: 1.25,
                        px: 2,
                      }}
                    >
                      I'm a Student
                    </Box>

                    <CardContent sx={{ p: 3, display: 'grid', gap: 1, placeItems: 'center' }}>
                      <Avatar src="/icons/reading-book.png" alt="Student" sx={{ width: 40, height: 40, bgcolor: 'secondary.main', mb: 1 }} />
                      <Typography variant="body2" color="text.secondary" sx={{ textAlign: 'center' }}>
                        Access your exercises, submit, and view feedback.
                      </Typography>
                    </CardContent>
                  </CardActionArea>
                </Card>
              </a>
            </Link>
          </Grid>

          {/* Instructor Card (secondary palette) */}
          <Grid size={{ xs: 12, md: 6 }}>
            <Link href={{ pathname: '/login/instructor' }} legacyBehavior passHref>
              <a style={{ textDecoration: 'none' }}>
                <Card
                  variant="outlined"
                  sx={{
                    borderRadius: '6px',
                    overflow: 'hidden',
                    borderColor: 'rgba(29, 78, 216, 0.10)',
                    boxShadow: '0 6px 20px rgba(29, 78, 216, 0.08)',
                    transition: 'transform 120ms ease, box-shadow 120ms ease',
                    '&:hover': {
                      transform: 'translateY(-2px)',
                      boxShadow: '0 10px 26px rgba(29, 78, 216, 0.12)'
                    }
                  }}
                >
                  <CardActionArea>
                    {/* Header bar to mirror homepage style */}
                    <Box
                      sx={{
                        background: 'linear-gradient(90deg,#43C6AC,#191654)',
                        color: 'secondary.contrastText',
                        textAlign: 'center',
                        fontWeight: 600,
                        fontSize: 14,
                        py: 1.25,
                        px: 2,
                      }}
                    >
                      I'm an Instructor
                    </Box>

                    <CardContent sx={{ p: 3, display: 'grid', gap: 1, placeItems: 'center' }}>
                      <Avatar src="/icons/teacher.png" alt="Instructor" sx={{ width: 40, height: 40, bgcolor: 'primary.main', mb: 1 }} />
                      <Typography variant="body2" color="text.secondary" sx={{ textAlign: 'center' }}>
                        Manage student submissions, grading, and reports.
                      </Typography>
                    </CardContent>
                  </CardActionArea>
                </Card>
              </a>
            </Link>
          </Grid>
        </Grid>

        {/* Divider echoing theme.divider */}
        <Box sx={{ mt: 4, height: 1, background: 'rgba(29, 78, 216, 0.18)' }} />
      </Box>
    </Box>
  );
}
