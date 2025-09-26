import nodemailer from 'nodemailer';

export type InviteEmailPayload = {
    to: string;
    name: string;
    link: string;
    fromOverride?: string;
};

function getTransport() {
    const host = process.env.SMTP_HOST;
    const port = process.env.SMTP_PORT ? Number(process.env.SMTP_PORT) : undefined;
    const user = process.env.SMTP_USER;
    const pass = process.env.SMTP_PASS;
    const secure = String(process.env.SMTP_SECURE || '').toLowerCase() === 'true';
    if (!host || !port || !user || !pass) return null;
    return nodemailer.createTransport({ host, port, secure, auth: { user, pass } });
}

export async function sendInviteEmail({ to, name, link, fromOverride }: InviteEmailPayload): Promise<{ sent: boolean; id?: string; error?: string }> {
    const from = fromOverride || process.env.SMTP_FROM || 'no-reply@socoles.local';
    const transporter = getTransport();
    const subject = `You're invited to join the class on SOCOLES`;
    const html = `
    <div style="font-family:system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell, Noto Sans, Helvetica, Arial, sans-serif; line-height:1.5;">
      <h2>You're invited${name ? ', ' + name : ''}!</h2>
      <p>Your instructor has invited you to join their class roster on SOCOLES.</p>
      <p><a href="${link}" style="display:inline-block;background:#1976d2;color:#fff;text-decoration:none;padding:10px 16px;border-radius:6px">Accept Invite</a></p>
      <p>If the button doesn't work, copy and paste this link into your browser:</p>
      <p><a href="${link}">${link}</a></p>
    </div>
  `;
    const text = `You're invited${name ? ', ' + name : ''}!\n\nOpen this link to accept: ${link}`;

    if (!transporter) {
        console.log('[email] SMTP not configured; would send invite:', { to, subject, link });
        return { sent: false, error: 'SMTP not configured' };
    }
    try {
        const info = await transporter.sendMail({ from, to, subject, html, text });
        return { sent: true, id: info.messageId };
    } catch (e: any) {
        return { sent: false, error: e?.message || 'send failed' };
    }
}
