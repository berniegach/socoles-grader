import * as React from 'react';
import { IconButton, Menu, MenuItem, ListItemIcon, ListItemText } from '@mui/material';
import MoreVertIcon from '@mui/icons-material/MoreVert';

export interface OverflowMenuAction {
    key: string;
    label: string;
    icon?: React.ReactNode;
    onClick?: (e: React.MouseEvent) => void;
    disabled?: boolean;
}

interface HeaderOverflowMenuProps {
    actions: OverflowMenuAction[];
}

const HeaderOverflowMenu: React.FC<HeaderOverflowMenuProps> = ({ actions }) => {
    const [anchorEl, setAnchorEl] = React.useState<null | HTMLElement>(null);
    const open = Boolean(anchorEl);

    const handleOpen = (event: React.MouseEvent<HTMLElement>) => {
        setAnchorEl(event.currentTarget);
    };
    const handleClose = () => {
        setAnchorEl(null);
    };

    const handleAction = (action: OverflowMenuAction) => (e: React.MouseEvent) => {
        handleClose();
        action.onClick?.(e);
    };

    return (
        <>
            <IconButton aria-label="More actions" onClick={handleOpen} size="small">
                <MoreVertIcon sx={{ color: '#fff' }} />
            </IconButton>
            <Menu anchorEl={anchorEl} open={open} onClose={handleClose} anchorOrigin={{ vertical: 'bottom', horizontal: 'right' }} transformOrigin={{ vertical: 'top', horizontal: 'right' }}>
                {actions.map((action) => (
                    <MenuItem key={action.key} onClick={handleAction(action)} disabled={action.disabled}>
                        {action.icon && <ListItemIcon>{action.icon}</ListItemIcon>}
                        <ListItemText>{action.label}</ListItemText>
                    </MenuItem>
                ))}
            </Menu>
        </>
    );
};

export default HeaderOverflowMenu;
