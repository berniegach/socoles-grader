"use client";
import { useEffect } from 'react';
import { EditorContent, useEditor } from '@tiptap/react';
import StarterKit from '@tiptap/starter-kit';
import Link from '@tiptap/extension-link';
import Image from '@tiptap/extension-image';
import { Box, Typography, Stack, IconButton, Tooltip, Divider, ToggleButtonGroup, ToggleButton } from '@mui/material';
import FormatBoldIcon from '@mui/icons-material/FormatBold';
import FormatItalicIcon from '@mui/icons-material/FormatItalic';
import StrikethroughSIcon from '@mui/icons-material/StrikethroughS';
import CodeIcon from '@mui/icons-material/Code';
import TitleIcon from '@mui/icons-material/Title';
import FormatListBulletedIcon from '@mui/icons-material/FormatListBulleted';
import FormatListNumberedIcon from '@mui/icons-material/FormatListNumbered';
import FormatQuoteIcon from '@mui/icons-material/FormatQuote';
import HorizontalRuleIcon from '@mui/icons-material/HorizontalRule';
import UndoIcon from '@mui/icons-material/Undo';
import RedoIcon from '@mui/icons-material/Redo';
import ClearIcon from '@mui/icons-material/ClearAll';
import InsertLinkIcon from '@mui/icons-material/InsertLink';
import ImageIcon from '@mui/icons-material/Image';
import FunctionsIcon from '@mui/icons-material/Functions';

export default function RichTextEditor({ value, onChange, label, minHeight = 220, placeholder = 'Write here…' }: { value: string; onChange: (html: string) => void; label?: string; minHeight?: number; placeholder?: string; }) {
    const editor = useEditor({
        extensions: [
            StarterKit.configure({ codeBlock: {} }),
            Link.configure({
                openOnClick: false,
                autolink: true,
                linkOnPaste: true,
                protocols: ['http', 'https', 'mailto', 'tel'],
                HTMLAttributes: { rel: 'noopener noreferrer nofollow', target: '_blank' },
            }),
            Image.configure({ inline: false, allowBase64: true }),
        ],
        content: '',
        immediatelyRender: false,
        editorProps: {
            attributes: { 'aria-label': label || 'Rich text editor' },
        },
        onUpdate({ editor }) {
            onChange(editor.getHTML());
        },
    });

    useEffect(() => {
        if (!editor) return;
        // Set initial content after mount to avoid SSR hydration issues
        const html = value || '';
        editor.commands.setContent(html, { emitUpdate: false });
        // run only once on mount
    }, [editor]);

    useEffect(() => {
        if (!editor) return;
        const cur = editor.getHTML();
        const next = value || '';
        if (cur !== next) editor.commands.setContent(next, { emitUpdate: false });
    }, [value, editor]);

    return (
        <Box sx={{ display: 'grid', gap: 0.5 }}>
            {label && <Typography variant='caption' sx={{ fontWeight: 600 }}>{label}</Typography>}
            <Box sx={{ border: '1px solid', borderColor: 'divider', borderRadius: 1, overflow: 'hidden' }}>
                {/* Toolbar */}
                <Box sx={{ p: 0.5, borderBottom: '1px solid', borderColor: 'divider', bgcolor: 'background.paper' }}>
                    <Stack direction="row" alignItems="center" spacing={0.5} flexWrap="wrap">
                        <Tooltip title="Bold">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('bold') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleBold().run()}
                                    disabled={!editor?.can().chain().focus().toggleBold().run()}
                                >
                                    <FormatBoldIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Italic">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('italic') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleItalic().run()}
                                    disabled={!editor?.can().chain().focus().toggleItalic().run()}
                                >
                                    <FormatItalicIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Strikethrough">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('strike') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleStrike().run()}
                                    disabled={!editor?.can().chain().focus().toggleStrike().run()}
                                >
                                    <StrikethroughSIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        {/* Headings */}
                        <Tooltip title="Headings">
                            <ToggleButtonGroup size="small" exclusive value={['1', '2', '3'].find(l => editor?.isActive('heading', { level: Number(l) })) ?? null}>
                                <ToggleButton value="1" onClick={() => editor?.chain().focus().toggleHeading({ level: 1 }).run()} selected={!!editor?.isActive('heading', { level: 1 })}>
                                    <TitleIcon fontSize="small" />1
                                </ToggleButton>
                                <ToggleButton value="2" onClick={() => editor?.chain().focus().toggleHeading({ level: 2 }).run()} selected={!!editor?.isActive('heading', { level: 2 })}>
                                    <TitleIcon fontSize="small" />2
                                </ToggleButton>
                                <ToggleButton value="3" onClick={() => editor?.chain().focus().toggleHeading({ level: 3 }).run()} selected={!!editor?.isActive('heading', { level: 3 })}>
                                    <TitleIcon fontSize="small" />3
                                </ToggleButton>
                            </ToggleButtonGroup>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        <Tooltip title="Bullet list">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('bulletList') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleBulletList().run()}
                                >
                                    <FormatListBulletedIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Ordered list">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('orderedList') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleOrderedList().run()}
                                >
                                    <FormatListNumberedIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Block quote">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('blockquote') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleBlockquote().run()}
                                >
                                    <FormatQuoteIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        {/* Links & Images */}
                        <Tooltip title={editor?.isActive('link') ? 'Edit link' : 'Add link'}>
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('link') ? 'primary' : 'default'}
                                    onClick={() => {
                                        if (!editor) return;
                                        const prev = editor.getAttributes('link')?.href as string | undefined;
                                        const url = window.prompt('Enter URL', prev ?? 'https://');
                                        if (url === null) return; // cancel
                                        if (url === '') {
                                            editor.chain().focus().unsetLink().run();
                                            return;
                                        }
                                        // ensure protocol
                                        const normalized = /^(https?:|mailto:|tel:)/.test(url) ? url : `https://${url}`;
                                        editor.chain().focus().extendMarkRange('link').setLink({ href: normalized }).run();
                                    }}
                                >
                                    <InsertLinkIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Insert image">
                            <span>
                                <IconButton
                                    size="small"
                                    onClick={() => {
                                        if (!editor) return;
                                        const src = window.prompt('Image URL (https://...)');
                                        if (!src) return;
                                        const alt = window.prompt('Alt text (optional)') ?? undefined;
                                        editor.chain().focus().setImage({ src, alt }).run();
                                    }}
                                >
                                    <ImageIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        {/* Math (TeX) helpers */}
                        <Tooltip title="Inline math: wrap with $...$">
                            <span>
                                <IconButton
                                    size="small"
                                    onClick={() => {
                                        if (!editor) return;
                                        const hasSelection = editor.state.selection.from !== editor.state.selection.to;
                                        if (hasSelection) {
                                            const tex = editor.state.doc.textBetween(editor.state.selection.from, editor.state.selection.to, '\n');
                                            editor.chain().focus().insertContent(`$${tex}$`).run();
                                        } else {
                                            editor.chain().focus().insertContent('$$').setTextSelection({ from: editor.state.selection.from + 1, to: editor.state.selection.from + 1 }).run();
                                        }
                                    }}
                                >
                                    <FunctionsIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Block math: insert $$\n...\n$$">
                            <span>
                                <IconButton
                                    size="small"
                                    onClick={() => {
                                        if (!editor) return;
                                        const template = "$$\n\\frac{a}{b} = c\n$$";
                                        editor.chain().focus().insertContent(`\n${template}\n`).run();
                                    }}
                                >
                                    <FunctionsIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        <Tooltip title="Inline code">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('code') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleCode().run()}
                                >
                                    <CodeIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Code block">
                            <span>
                                <IconButton
                                    size="small"
                                    color={editor?.isActive('codeBlock') ? 'primary' : 'default'}
                                    onClick={() => editor?.chain().focus().toggleCodeBlock().run()}
                                >
                                    <CodeIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Horizontal rule">
                            <span>
                                <IconButton size="small" onClick={() => editor?.chain().focus().setHorizontalRule().run()}>
                                    <HorizontalRuleIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        <Tooltip title="Undo">
                            <span>
                                <IconButton size="small" onClick={() => editor?.chain().focus().undo().run()} disabled={!editor?.can().chain().focus().undo().run()}>
                                    <UndoIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                        <Tooltip title="Redo">
                            <span>
                                <IconButton size="small" onClick={() => editor?.chain().focus().redo().run()} disabled={!editor?.can().chain().focus().redo().run()}>
                                    <RedoIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>

                        <Divider flexItem orientation="vertical" sx={{ mx: 0.5 }} />

                        <Tooltip title="Clear formatting">
                            <span>
                                <IconButton size="small" onClick={() => editor?.chain().focus().unsetAllMarks().clearNodes().run()}>
                                    <ClearIcon fontSize="small" />
                                </IconButton>
                            </span>
                        </Tooltip>
                    </Stack>
                </Box>

                {/* Editor */}
                <Box sx={{ p: 1, '& .ProseMirror': { minHeight, outline: 'none' }, '& .is-empty::before': { content: `'${placeholder}'`, color: 'text.disabled' } }}>
                    <EditorContent editor={editor} />
                </Box>
            </Box>
        </Box>
    );
}
