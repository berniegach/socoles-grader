export function formatDateTimeDDMMYYYYHHmm(input?: string) {
    if (!input) return '';
    // Accept ISO or space-separated, normalize to Date
    const d = new Date(input);
    if (isNaN(d.getTime())) return input;
    const pad = (n: number) => (n < 10 ? `0${n}` : String(n));
    const dd = pad(d.getDate());
    const mm = pad(d.getMonth() + 1);
    const yyyy = d.getFullYear();
    const hh = pad(d.getHours());
    const min = pad(d.getMinutes());
    return `${dd}-${mm}-${yyyy} ${hh}:${min}`;
}
