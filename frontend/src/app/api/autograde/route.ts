import { NextRequest, NextResponse } from 'next/server';

// POST /api/autograde
// Server-side proxy that forwards autograder requests to the backend using the Docker internal network.
export async function POST(req: NextRequest) {
    try {
        const base = process.env.SOCOLES_INTERNAL_API_URL || 'http://localhost:5000';
        const path = process.env.NEXT_PUBLIC_SOCOLES_GRADE_PATH || '/grade-queries';
        const url = `${base}${path}`;
        const payload = await req.json();

        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 25000);
        try {
            const resp = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload),
                signal: controller.signal,
            });
            const contentType = resp.headers.get('content-type') || 'application/json';
            const text = await resp.text();
            return new NextResponse(text, { status: resp.status, headers: { 'content-type': contentType } });
        } finally {
            clearTimeout(timeout);
        }
    } catch (e: unknown) {
        const msg = e instanceof Error ? e.message : 'failed';
        return NextResponse.json({ error: msg }, { status: 500 });
    }
}
