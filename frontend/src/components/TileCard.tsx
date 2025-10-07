
import React from "react";
import { Box, Typography } from "@mui/material";
import { SxProps, Theme } from "@mui/material/styles";

interface TileCardProps {
    title: React.ReactNode;
    subtitle?: React.ReactNode;
    chips?: React.ReactNode[];
    headerRight?: React.ReactNode;
    children?: React.ReactNode;
    onClick?: () => void;
    tabIndex?: number;
    role?: string;
    sx?: SxProps<Theme>;
    [key: string]: any;
}

const TileCard: React.FC<TileCardProps> = ({
    title,
    subtitle,
    chips,
    headerRight,
    children,
    onClick,
    tabIndex = 0,
    role = "button",
    sx,
    ...rest
}) => {
    const sxArray = [
        (theme: Theme) => ({
            position: "relative",
            p: 1.25,
            borderRadius: 1.25,
            cursor: onClick ? "pointer" : undefined,
            display: "grid",
            gap: 0.75,
            minHeight: 108,
            background:
                theme.palette.mode === "dark"
                    ? "rgba(255,255,255,0.03)"
                    : "linear-gradient(145deg,#ffffff,#f9f9f9)",
            border: "1px solid",
            borderColor:
                theme.palette.mode === "dark"
                    ? "rgba(255,255,255,0.08)"
                    : "rgba(0,0,0,0.08)",
            boxShadow: "0 1px 2px rgba(0,0,0,0.08)",
            transition:
                "background .25s, box-shadow .25s, transform .25s, border-color .25s",
            outline: "none",
            "&:hover": {
                ...(onClick
                    ? {
                        boxShadow: "0 4px 12px -2px rgba(0,0,0,0.25)",
                        transform: "translateY(-2px)",
                        borderColor: theme.palette.primary.light,
                    }
                    : {}),
            },
            "&:focus-visible": {
                boxShadow: "0 0 0 2px #fff, 0 0 0 4px #ff66c4",
            },
        }),
        ...(Array.isArray(sx) ? sx : sx ? [sx] : []),
    ];
    return (
        <Box
            tabIndex={tabIndex}
            role={role}
            onClick={onClick}
            sx={sxArray}
            {...rest}
        >
            <Box sx={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", gap: 1 }}>
                <Box sx={{ flex: 1, minWidth: 0 }}>
                    <Typography variant="subtitle2" sx={{ fontWeight: 600, lineHeight: 1.2, pr: 1, overflow: "hidden", textOverflow: "ellipsis" }}>{title}</Typography>
                    {subtitle && (
                        <Typography variant="caption" color="text.secondary">
                            {subtitle}
                        </Typography>
                    )}
                </Box>
                {headerRight}
            </Box>
            {chips && chips.length > 0 && (
                <Box sx={{ display: "flex", alignItems: "center", gap: 0.6, flexWrap: "wrap" }}>{chips}</Box>
            )}
            {children}
        </Box>
    );
};

export default TileCard;
