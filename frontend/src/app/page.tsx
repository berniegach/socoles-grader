'use client';
import AppShell from '@/components/AppShell';
import Login from '@/features/auth/Login';
import { useAuth } from '@/features/auth/AuthProvider';

export default function HomePage() {
  const { user } = useAuth();
  // Show login if no instructor token AND not a student
  if (!user) return <Login />;
  return <AppShell />;
}
