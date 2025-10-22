# Keycloak Quickstart (Docker Compose)

This repository includes an optional Keycloak container to make SSO login easy for anyone running `docker compose up`.

## What this gives you

- A Keycloak server on http://keycloak:8080 (published at http://localhost:8080)
- A pre-imported realm: `socoles`
- A pre-created OIDC client: `socoles-frontend` (confidential)
- Valid redirect URIs (local):
  - http://localhost:3000/auth/student/callback
  - http://localhost:3000/auth/instructor/callback
- Post logout redirect URI: /

## One-time setup (host machine)

Because the app (server) and the browser must use the same Issuer URL, we pick `http://keycloak:8080/…` and make the browser resolve `keycloak` to localhost.

Add this line to your host `/etc/hosts` (needs sudo):

```
127.0.0.1 keycloak
```

This lets the browser open http://keycloak:8080 and reach the container exposed at 127.0.0.1:8080. The Next.js server inside Docker already reaches `keycloak:8080` via the Docker network.

## Environment variables

In `.env` (copied from `.env.example`), set:

```
NEXTAUTH_URL=http://localhost:3000
NEXTAUTH_SECRET=<a-long-random-string>
KEYCLOAK_ISSUER=http://keycloak:8080/realms/socoles
KEYCLOAK_CLIENT_ID=socoles-frontend
KEYCLOAK_CLIENT_SECRET=change-me   # update to the secret you want
KEYCLOAK_LOGOUT_REDIRECT_URI=http://localhost:3000
```

You can change the Keycloak admin credentials by setting `KEYCLOAK_ADMIN` and `KEYCLOAK_ADMIN_PASSWORD` in `.env`.

## Start services

```
docker compose up -d
```

Open the admin console at http://keycloak:8080/ and login with the admin credentials. You’ll see the `socoles` realm and `socoles-frontend` client imported.

## Production notes

- Replace `http://localhost:3000` with your public origin (e.g., `https://socoles.cs.ru.nl`) in NEXTAUTH_URL and KEYCLOAK_LOGOUT_REDIRECT_URI.
- Update the client’s Valid Redirect URIs and Post Logout Redirect URIs to match your domain.
- Rotate `NEXTAUTH_SECRET` to a long, random value and keep it stable.
- Change the Keycloak client secret and admin credentials.
