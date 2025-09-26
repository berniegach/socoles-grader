# Multi-stage Dockerfile for Next.js frontend (production build)
# 1. Install dependencies (with caching) using pnpm/npm (here npm since lockfile is package-lock.json)
# 2. Build the Next.js app
# 3. Run with a minimal runtime image

# ---- Base deps image ----
FROM node:18-bullseye AS deps
WORKDIR /app

# Install dependencies (only package*.json first for better caching)
COPY package.json package-lock.json ./
RUN npm ci

# ---- Build stage ----
FROM node:18-bullseye AS build
WORKDIR /app
ENV NEXT_TELEMETRY_DISABLED=1

COPY --from=deps /app/node_modules ./node_modules
COPY . .

# Build the Next.js application
RUN npm run build

# ---- Production runtime ----
FROM node:18-bullseye AS runner
WORKDIR /app
ENV NODE_ENV=production \
    NEXT_TELEMETRY_DISABLED=1

# Prevent Next.js from attempting to write telemetry

# Copy only necessary output
COPY --from=build /app/package.json ./
COPY --from=build /app/package-lock.json ./
COPY --from=build /app/node_modules ./node_modules
COPY --from=build /app/.next ./.next
COPY --from=build /app/public ./public

# Expose Next.js default port
EXPOSE 3000

# Set the command to run the production server
CMD ["npm", "run", "start"]

# For local dev you can instead build an image from the first Dockerfile or override the command:
# docker build -t grader-frontend .
# docker run -p 3000:3000 grader-frontend
