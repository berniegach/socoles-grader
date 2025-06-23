// src/components/FileUploadBox.js

import React from 'react';
import {
  Box,
  Typography,
  Stack,
  IconButton,
  Tooltip,
} from '@mui/material';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import DeleteIcon from '@mui/icons-material/Delete';
import { useDropzone } from 'react-dropzone';

function FileUploadBox({ label, fileType, file, onFileDrop, onRemoveFile, error, accept }) {
  const { getRootProps, getInputProps, isDragActive } = useDropzone({
    onDrop: onFileDrop(fileType),
    accept,
    multiple: false,
  });

  return (
    <Box
      sx={{
        border: '2px dashed #ccc',
        borderRadius: 2,
        p: 2,
        position: 'relative',
        textAlign: 'center',
        maxWidth: 300,
        width: '100%',
        mx: 'auto',
      }}
    >
      <Box
        {...getRootProps()}
        sx={{
          p: 2,
          backgroundColor: isDragActive ? '#f0f8ff' : 'transparent',
          cursor: 'pointer',
        }}
      >
        <input {...getInputProps()} />
        <Stack direction="column" alignItems="center" spacing={1}>
          <UploadFileIcon fontSize="large" />
          <Typography variant="subtitle1">
            {file ? file.name : `Upload ${label}`}
          </Typography>
          <Typography variant="body2" color="textSecondary">
            {`${
              label === 'Auto DB SQL File'
                ? 'SQL file containing queries to create the tables and data.'
                : label === 'Reference Queries'
                ? 'CSV file containing the correct queries.'
                : label === 'Student Queries'
                ? 'CSV file containing students queries.'
                : 'Upload the required file.'
            }`}
          </Typography>
        </Stack>
      </Box>

      {/* Remove File Button */}
      {file && (
        <Tooltip title="Remove File">
          <IconButton
            color="error"
            onClick={onRemoveFile(fileType)}
            sx={{ position: 'absolute', top: 8, right: 8 }}
          >
            <DeleteIcon />
          </IconButton>
        </Tooltip>
      )}

      {/* Display validation error */}
      {error && (
        <Typography variant="body2" color="error" sx={{ mt: 1 }}>
          {error}
        </Typography>
      )}
    </Box>
  );
}

export default FileUploadBox;
