/** @type {import('next').NextConfig} */
const nextConfig = {
  reactStrictMode: true,
  // typedRoutes moved from experimental in Next 15
  typedRoutes: true,
  // Allow build to proceed even if ESLint finds warnings/errors.
  // We rely on separate lint script in CI.
  eslint: { ignoreDuringBuilds: true },
};
export default nextConfig;