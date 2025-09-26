"use client";
import React from 'react';
import { Box, Typography } from '@mui/material';
import CodeMirror from '@uiw/react-codemirror';
import { sql as sqlLang } from '@codemirror/lang-sql';

export type SqlEditorProps = {
  label?: string;
  value: string;
  onChange: (v: string) => void;
  placeholder?: string;
  minRows?: number; // approximate rows; container is resizable
  maxHeight?: number;
  onEnterSubmit?: () => void;
  readOnly?: boolean;
};

/**
 * Reusable SQL editor with CodeMirror + SQL highlighting.
 * - Resizable container with monospace styling
 * - Optional label and Enter-to-submit callback
 * - Read-only mode supported
 */
export default function SqlEditor({ label, value, onChange, placeholder, minRows = 8, maxHeight = 600, onEnterSubmit, readOnly = false }: SqlEditorProps) {
  const line = 22; // px per line approx
  const minH = Math.max(3, minRows) * line + 16; // padding headroom
  return (
    <Box sx={{ display: 'grid', gap: 0.5 }}>
      {label && (
        <Typography variant='caption' sx={{ fontWeight: 600 }}>{label}</Typography>
      )}
      <Box
        sx={{
          border: '1px solid',
          borderColor: 'divider',
          borderRadius: 1,
          overflow: 'hidden',
          resize: 'vertical',
          minHeight: minH,
          maxHeight: maxHeight,
          '& .cm-editor': {
            height: '100%',
            fontSize: 13,
            fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, monospace',
            backgroundColor: 'background.paper',
          },
          '& .cm-scroller': { overflow: 'auto' },
          '& .cm-gutters': { borderRightColor: 'divider' },
        }}
      >
        <CodeMirror
          value={value}
          height='100%'
          extensions={[sqlLang()]}
          placeholder={placeholder}
          basicSetup={{ lineNumbers: true, highlightActiveLine: false, foldGutter: true, bracketMatching: true, autocompletion: true }}
          onChange={(v) => onChange(v)}
          onKeyDown={(e: any) => {
            if (!readOnly && onEnterSubmit && e?.key === 'Enter' && !e.shiftKey) {
              e.preventDefault?.();
              onEnterSubmit();
            }
          }}
          editable={!readOnly}
        />
      </Box>
    </Box>
  );
}
