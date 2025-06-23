// src/Grade.js

import React, { useState } from 'react';
import { Box, Stepper, Step, StepLabel, Button, Stack, CircularProgress, Backdrop } from '@mui/material';
import axios from 'axios';
import UploadFiles from './UploadFiles';
import GradingOptions from './GradingOptions';
import Results from './Results';
import SnackbarAlert from './SnackbarAlert';
import Papa from 'papaparse';

function Grade() {
  // Stepper Steps
  const steps = ['Upload Files', 'Grading Options', 'Results'];

  const initialFilesState = {
    sqlFile: null,
    referenceFile: null,
    studentFile: null,
    autoDBSqlFile: null,
  };

  const initialOptionsState = {
    syntaxSensitivity: '',
    semanticsSensitivity: '',
    resultsSensitivity: '',
    evaluationPriority: '',
    textEditDistance: '',
    treeEditDistance: '',
    checkOrder: false,
    autoDB: false,
    numberOfDBs: '',
    dbName: '',
    use_postgresql: true,
  };

  const initialErrorsState = {
    sqlFile: '',
    referenceFile: '',
    studentFile: '',
    autoDBSqlFile: '',
  };

  // State declarations
  const [files, setFiles] = useState(initialFilesState); // State for file uploads
  const [options, setOptions] = useState(initialOptionsState);
  const [errors, setErrors] = useState(initialErrorsState);
  const [activeStep, setActiveStep] = useState(0);
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [gradingResults, setGradingResults] = useState(null);
  const [snackbar, setSnackbar] = useState({ open: false, message: '', severity: '' });
  const [questionNumbers, setQuestionNumbers] = useState([]);
  const [selectedQuestionNumber, setSelectedQuestionNumber] = useState('');

  const handleRestart = () => {
    // Reset all states to initial values
    setFiles(initialFilesState);
    setOptions(initialOptionsState);
    setErrors(initialErrorsState);
    setActiveStep(0);
    setIsSubmitting(false);
    setGradingResults(null);
    setQuestionNumbers([]);
    setSelectedQuestionNumber('');
    // You may also want to reset the snackbar
    setSnackbar({ open: false, message: '', severity: '' });
  };



  // Handler for closing Snackbar
  const handleCloseSnackbar = () => {
    setSnackbar({ ...snackbar, open: false });
  };


  // Read files and convert them to appropriate formats
  const readFileAsText = (file) => {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = (event) => {
        resolve(event.target.result);
      };
      reader.onerror = (error) => {
        reject(error);
      };
      reader.readAsText(file);
    });
  };

  // Function to parse the student queries CSV file and extract question numbers
  const parseStudentQueriesFile = (file) => {
    readFileAsText(file)
      .then((content) => {
        const parsedData = Papa.parse(content, {
          header: true,
          skipEmptyLines: true,
        });

        // Extract unique question numbers
        const qNumbersSet = new Set();
        parsedData.data.forEach((row) => {
          if (row['Q #']) {
            qNumbersSet.add(row['Q #'].trim());
          }
        });

        // Update the state with the list of question numbers
        setQuestionNumbers(Array.from(qNumbersSet).sort());
      })
      .catch((error) => {
        console.error('Error parsing student queries CSV file:', error);
        setSnackbar({
          open: true,
          message: 'Error parsing student queries CSV file.',
          severity: 'error',
        });
      });
  };

  // Handler for file uploads
  const handleFileDrop = (fileType) => (acceptedFiles) => {
    if (acceptedFiles.length > 0) {
      const file = acceptedFiles[0];
      setFiles((prev) => ({ ...prev, [fileType]: file }));
      validateFile(fileType, file);

      // Additional processing for Student Queries CSV file
      if (fileType === 'studentFile') {
        parseStudentQueriesFile(file);
      }

      // Additional processing for SQL files
      if (fileType === 'sqlFile' || fileType === 'autoDBSqlFile') {
        const reader = new FileReader();
        reader.onload = (event) => {
          // Assuming you might want to store SQL data if needed
          // For now, no action is taken
        };
        reader.onerror = (error) => {
          console.error('Error reading the file:', error);
          setSnackbar({
            open: true,
            message: 'Error reading the SQL file.',
            severity: 'error',
          });
        };
        reader.readAsText(file);
      }
    }
  };

  // Validation function
  const validateFile = (fileType, file) => {
    let error = '';

    if (fileType === 'referenceFile') {
      // Validate Reference Queries: CSV with only one column
      if (
        file.type !== 'text/csv' &&
        !file.name.toLowerCase().endsWith('.csv')
      ) {
        error = 'Reference Queries must be a CSV file.';
      } else {
        // Additional validation for single column can be implemented here
      }
    }

    if (fileType === 'studentFile') {
      // Validate Student Queries: CSV with specific columns
      if (
        file.type !== 'text/csv' &&
        !file.name.toLowerCase().endsWith('.csv')
      ) {
        error = 'Student Queries must be a CSV file.';
      } else {
        // Additional validation for required columns can be implemented here
      }
    }

    if (fileType === 'sqlFile' || fileType === 'autoDBSqlFile') {
      // Validate SQL files
      if (
        file.type !== 'application/sql' &&
        file.type !== 'text/sql' &&
        !file.name.toLowerCase().endsWith('.sql')
      ) {
        error = 'SQL File must be a .sql file.';
      }
    }

    setErrors((prev) => ({ ...prev, [fileType]: error }));

    // After validation, check if all required files are valid
    const updatedFiles = { ...files, [fileType]: file };
    const validationErrors = { ...errors, [fileType]: error };

    // Determine required files based on Auto DB option
    const requiredFiles = ['referenceFile', 'studentFile', 'sqlFile'];
    if (options.autoDB) {
      requiredFiles.push('autoDBSqlFile');
    }

    const isValid = requiredFiles.every(
      (type) => updatedFiles[type] && validationErrors[type] === ''
    );

  };

  // Handler to remove a file
  const handleRemoveFile = (fileType) => () => {
    setFiles((prev) => ({ ...prev, [fileType]: null }));
    setErrors((prev) => ({ ...prev, [fileType]: '' }));
    setSnackbar({
      open: true,
      message: `${getFileLabel(fileType)} removed successfully.`,
      severity: 'info',
    });
  };

  // Helper to get file labels
  const getFileLabel = (fileType) => {
    switch (fileType) {
      case 'referenceFile':
        return 'Reference Queries';
      case 'studentFile':
        return 'Student Queries';
      case 'sqlFile':
        return 'SQL File';
      case 'autoDBSqlFile':
        return 'Auto DB SQL File';
      default:
        return '';
    }
  };

  // Handler for option changes
  const handleOptionChange = (optionName) => (event) => {
    const value =
      event.target.type === 'checkbox'
        ? event.target.checked
        : event.target.value;
    setOptions((prev) => ({ ...prev, [optionName]: value }));
  };

  // Handler for "Next" button
  const handleNext = () => {
    // Validate that all required files are uploaded correctly
    const requiredFiles = ['referenceFile', 'studentFile', 'sqlFile'];
    if (options.autoDB) {
      requiredFiles.push('autoDBSqlFile');
    }

    const isValid = requiredFiles.every(
      (type) => files[type] && errors[type] === ''
    );

    if (isValid) {
      setActiveStep((prev) => prev + 1);
    } else {
      setSnackbar({
        open: true,
        message:
          'Please ensure all required files are uploaded correctly before proceeding.',
        severity: 'warning',
      });
    }
  };

  // Handler for "Back" button
  const handleBack = () => {
    setActiveStep((prev) => prev - 1);
  };

  // Handler for form submission
  const handleSubmit = async () => {
    setIsSubmitting(true);

    try {
      // Read the SQL file (sqlFile)
      const sqlData = await readFileAsText(files.sqlFile);

      // Read the Reference Queries CSV file (referenceFile)
      const referenceFileContent = await readFileAsText(files.referenceFile);
      // Parse CSV to extract model queries
      const modelQueries = referenceFileContent
        .split('\n')
        .map((line) => line.trim())
        .filter((line) => line);

      // Read the Student Queries CSV file (studentFile)
      const studentFileContent = await readFileAsText(files.studentFile);
      // Use Papa Parse to parse the CSV content
      const parsedStudentData = Papa.parse(studentFileContent, {
        header: true,
        skipEmptyLines: true,
      });

      // Extract the required fields and filter by selected question number
      const studentQueries = parsedStudentData.data
        .filter((row) => row['Q #'] && row['Q #'].trim() === selectedQuestionNumber)
        .map((row) => {
          return [
            row['Org Defined ID'].trim(),
            parseInt(row['Attempt #'], 10),
            row['Q #'].trim(),
            row['Answer'].trim(),
          ];
        });

      // If Auto DB is enabled, read the Auto DB SQL file
      let sqlCreate = '';
      if (options.autoDB && files.autoDBSqlFile) {
        sqlCreate = await readFileAsText(files.autoDBSqlFile);
      }

      // Prepare the data payload
      const dataPayload = {
        sql_data: sqlData, // Contents of the SQL file
        queries: studentQueries, // Student queries array
        model_queries: modelQueries, // Model queries array
        syntax: parseInt(options.syntaxSensitivity, 10),
        semantics: parseInt(options.semanticsSensitivity, 10),
        results: parseInt(options.resultsSensitivity, 10),
        prop_order: parseInt(options.evaluationPriority, 10),
        edit_dist: parseInt(options.textEditDistance, 10) || 0,
        tree_dist: parseInt(options.treeEditDistance, 10) || 0,
        check_order: options.checkOrder ? 1 : 0,
        auto_db: options.autoDB ? 1 : 0,
        use_postgresql: options.use_postgresql,
        num_db: parseInt(options.numberOfDBs, 10) || 0,
        sql_create_data: sqlCreate, // Contents of the Auto DB SQL file
        dbname: options.dbName || '',
      };

      // Send the data to the backend
      const response = await axios.post('http://localhost:5000/grade-queries', dataPayload, {
        headers: {
          'Content-Type': 'application/json',
        },
      });

      // Assuming the response data is the grading results
      setGradingResults(response.data);
      setActiveStep(2); // Move to Results step
      setSnackbar({
        open: true,
        message: 'Grading completed successfully!',
        severity: 'success',
      });
    } catch (error) {
      console.error('Error submitting grading data:', error);
      setSnackbar({
        open: true,
        message: 'Failed to submit grading data.',
        severity: 'error',
      });
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <Box sx={{ width: '100%', p: { xs: 2, sm: 4, md: 6 } }}>
      <Stepper activeStep={activeStep} alternativeLabel>
        {steps.map((label) => (
          <Step key={label}>
            <StepLabel>{label}</StepLabel>
          </Step>
        ))}
      </Stepper>

      {/* Step Content */}
      {activeStep === 0 && (
        <>
          <UploadFiles
            files={files}
            errors={errors}
            onFileDrop={handleFileDrop}
            onRemoveFile={handleRemoveFile}
            options={options}
            handleOptionChange={handleOptionChange}
          />
          {/* Navigation Button */}
          <Box sx={{ mt: 4, textAlign: 'center' }}>
            <Button
              variant="contained"
              color="primary"
              onClick={handleNext}
              disabled={
                !files.referenceFile ||
                !files.studentFile ||
                !files.sqlFile ||
                (options.autoDB && !files.autoDBSqlFile)
              }
            >
              Next
            </Button>
          </Box>
        </>
      )}
      {activeStep === 1 && (
        <>
          <GradingOptions
            options={options}
            handleOptionChange={handleOptionChange}
            questionNumbers={questionNumbers}
            selectedQuestionNumber={selectedQuestionNumber}
            setSelectedQuestionNumber={setSelectedQuestionNumber}
          />
          {/* Navigation Buttons */}
          <Box sx={{ mt: 4, textAlign: 'center' }}>
            <Stack direction="row" spacing={2} justifyContent="center">
              <Button variant="outlined" onClick={handleBack}>
                Back
              </Button>
              <Button
                variant="contained"
                color="primary"
                onClick={handleSubmit}
                disabled={
                  isSubmitting ||
                  !options.syntaxSensitivity ||
                  !options.semanticsSensitivity ||
                  !options.resultsSensitivity ||
                  !options.evaluationPriority ||
                  !selectedQuestionNumber ||
                  (options.autoDB && (!options.dbName || !options.numberOfDBs))
                }
              >
                Submit
              </Button>
            </Stack>
          </Box>
        </>
      )}
      {activeStep === 2 && (
        <Results
          gradingResults={gradingResults}
          onBack={handleBack}
          onRestart={handleRestart}
          studentFile={files.studentFile}
        />
      )}

      {/* Backdrop with CircularProgress */}
      <Backdrop
        sx={{ color: '#fff', zIndex: (theme) => theme.zIndex.drawer + 1 }}
        open={isSubmitting}
      >
        <CircularProgress color="inherit" />
      </Backdrop>

      {/* Snackbar for Notifications */}
      <SnackbarAlert
        open={snackbar.open}
        message={snackbar.message}
        severity={snackbar.severity}
        onClose={handleCloseSnackbar}
      />
    </Box>
  );
}

export default Grade;
