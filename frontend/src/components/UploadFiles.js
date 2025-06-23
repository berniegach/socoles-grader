// src/components/UploadFiles.js

import React from 'react';
import {
  Box,
  Typography,
  Grid,
  LinearProgress,
  Container,
} from '@mui/material';
import FileUploadBox from './FileUploadBox';

function UploadFiles({
  files,
  errors,
  onFileDrop,
  onRemoveFile,
  options,
  handleOptionChange,
}) {
  // Determine required files based on Auto DB option
  const requiredFiles = ['referenceFile', 'studentFile', 'sqlFile'];
  if (options.autoDB) {
    requiredFiles.push('autoDBSqlFile');
  }

  const uploadedFilesCount = requiredFiles.filter((type) => files[type]).length;
  const progressValue = (uploadedFilesCount / requiredFiles.length) * 100;

  return (
    <Container maxWidth="lg" sx={{ mt: 6 }}>
      <Grid container spacing={4} justifyContent="center">
        {/* Reference Queries */}
        <Grid item xs={12} sm={6} md={4}>
          <FileUploadBox
            label="Reference Queries"
            fileType="referenceFile"
            file={files.referenceFile}
            onFileDrop={onFileDrop}
            onRemoveFile={onRemoveFile}
            error={errors.referenceFile}
            accept={{ 'text/csv': ['.csv'] }}
          />
        </Grid>

        {/* Student Queries */}
        <Grid item xs={12} sm={6} md={4}>
          <FileUploadBox
            label="Student Queries"
            fileType="studentFile"
            file={files.studentFile}
            onFileDrop={onFileDrop}
            onRemoveFile={onRemoveFile}
            error={errors.studentFile}
            accept={{ 'text/csv': ['.csv'] }}
          />
        </Grid>

        {/* SQL File */}
        <Grid item xs={12} sm={6} md={4}>
          <FileUploadBox
            label="SQL File"
            fileType="sqlFile"
            file={files.sqlFile}
            onFileDrop={onFileDrop}
            onRemoveFile={onRemoveFile}
            error={errors.sqlFile}
            accept={{
              'application/sql': ['.sql'],
              'text/sql': ['.sql'],
            }}
          />
        </Grid>

        {/* Auto DB SQL File (Conditional) */}
        {options.autoDB && (
          <Grid item xs={12} sm={6} md={4}>
            <FileUploadBox
              label="Auto DB SQL File"
              fileType="autoDBSqlFile"
              file={files.autoDBSqlFile}
              onFileDrop={onFileDrop}
              onRemoveFile={onRemoveFile}
              error={errors.autoDBSqlFile}
              accept={{
                'application/sql': ['.sql'],
                'text/sql': ['.sql'],
              }}
            />
          </Grid>
        )}
      </Grid>

      {/* Progress Bar */}
      <Box mt={4} sx={{ textAlign: 'center' }}>
        <Typography variant="subtitle1">
          {requiredFiles.length - uploadedFilesCount} file(s) remaining to attach
        </Typography>
        <Box sx={{ mt: 2, maxWidth: 600, mx: 'auto' }}>
          <LinearProgress
            variant="determinate"
            value={progressValue}
            sx={{ height: 10, borderRadius: 5 }}
          />
        </Box>
      </Box>
    </Container>
  );
}

export default UploadFiles;
