"use client";

import { useLayoutEffect, useMemo, useRef } from 'react';
import { Box, SxProps, Theme } from '@mui/material';
import DOMPurify from 'dompurify';

type Props = { html: string; sx?: SxProps<Theme> };

export default function SafeRichText({ html, sx }: Props) {
    const ref = useRef<HTMLDivElement | null>(null);
    const clean = useMemo(() => DOMPurify.sanitize(html || ''), [html]);

    // Ensure KaTeX runs after each render and after transitions (Collapse)
    useLayoutEffect(() => {
        let cancelled = false;
        const el = ref.current;

        async function renderNow(target: HTMLElement) {
            try {
                const mod: any = await import('katex/contrib/auto-render');
                const render = mod?.default || mod?.renderMathInElement;
                if (!render || cancelled) return;
                render(target, {
                    delimiters: [
                        { left: '$$', right: '$$', display: true },
                        { left: '\\[', right: '\\]', display: true },
                        { left: '$', right: '$', display: false },
                        { left: '\\(', right: '\\)', display: false },
                    ],
                    throwOnError: false,
                    strict: 'ignore',
                });
            } catch {
                // ignore
            }
        }

        if (el) {
            // If content already rendered by KaTeX, skip immediate re-run
            const already = el.querySelector('.katex');
            if (!already) {
                void renderNow(el);
            }
            // Next tick to catch DOM after animations
            const t = setTimeout(() => {
                if (!cancelled && ref.current && !ref.current.querySelector('.katex')) {
                    void renderNow(ref.current as HTMLElement);
                }
            }, 0);
            return () => { cancelled = true; clearTimeout(t); };
        }
        return () => { cancelled = true; };
        // Intentionally no deps to run on any parent re-render
        // eslint-disable-next-line react-hooks/exhaustive-deps
    });

    return <Box ref={ref} sx={sx} dangerouslySetInnerHTML={{ __html: clean }} />;
}
