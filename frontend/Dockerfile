# Stage 1: build the React/Node frontend
FROM node:18-bullseye AS frontend-builder

WORKDIR /app

# 1. Install dependencies
COPY package.json package-lock.json ./
RUN npm ci

# 2. Build production assets
COPY . .

# Allow CRA’s dev server to bind to all interfaces
ENV HOST=0.0.0.0

# Expose the port that npm start uses by default
EXPOSE 3000

# Run the same start command you use locally
CMD ["npm", "start"]