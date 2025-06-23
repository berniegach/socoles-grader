// src/components/QueryOverview.js

import React, { useState } from 'react';
import {
  Box,
  Typography,
  Button,
  Stack,
  LinearProgress,
  Paper,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Snackbar,
  Alert,
  Container,
  Grid,
  Card,
  useTheme,
} from '@mui/material';
import UploadFileIcon from '@mui/icons-material/UploadFile';
import Papa from 'papaparse';
import axios from 'axios';
import FlagIcon from '@mui/icons-material/Flag';
import AdjustIcon from '@mui/icons-material/Adjust';
import SearchIcon from '@mui/icons-material/Search';
import FileUploadBox from './FileUploadBox'; // Adjust the import path as needed

function QueryOverview() {
  const theme = useTheme();
  const [csvFile, setCsvFile] = useState(null);
  const [queries, setQueries] = useState([]);
  const [parsedData, setParsedData] = useState({});
  const [loading, setLoading] = useState(false);
  const [snackbar, setSnackbar] = useState({
    open: false,
    message: '',
    severity: 'success', // 'error', 'warning', 'info'
  });

  // Handler for file selection via FileUploadBox
  const handleFileDrop = (fileType) => (acceptedFiles) => {
    if (acceptedFiles.length > 0) {
      console.log(`File selected for ${fileType}:`, acceptedFiles[0].name);
      if (fileType === 'referenceQueries') {
        setCsvFile(acceptedFiles[0]);
        // Reset queries and parsedData when a new file is selected
        setQueries([]);
        setParsedData({});
      }
      // Handle other file types if needed
    }
  };

  // Handler to remove the selected file
  const handleRemoveFile = (fileType) => () => {
    if (fileType === 'referenceQueries') {
      setCsvFile(null);
      setQueries([]);
      setParsedData({});
      setSnackbar({
        open: true,
        message: 'File removed successfully.',
        severity: 'info',
      });
    }
    // Handle other file types if needed
  };

  // Handler to process the selected CSV file
  const handleProcessFile = () => {
    console.log('Process File button clicked');
    if (csvFile) {
      console.log('CSV file selected:', csvFile.name);
      Papa.parse(csvFile, {
        header: false,
        skipEmptyLines: true,
        complete: (results) => {
          const extractedQueries = results.data.map((row) => row[0].trim()).filter(Boolean);
          console.log('Extracted Queries:', extractedQueries);
          if (extractedQueries.length === 0) {
            setSnackbar({
              open: true,
              message: 'The CSV file is empty or contains only empty rows.',
              severity: 'warning',
            });
            return;
          }
          setQueries(extractedQueries);
          fetchParsedData(extractedQueries);
        },
        error: (error) => {
          console.error('Error parsing CSV:', error);
          setSnackbar({
            open: true,
            message: 'Failed to parse the CSV file.',
            severity: 'error',
          });
        },
      });
    } else {
      console.log('No CSV file selected');
      setSnackbar({
        open: true,
        message: 'Please select a CSV file first.',
        severity: 'warning',
      });
    }
  };

  // Function to send queries to the backend and fetch parsed data
  const fetchParsedData = async (queries) => {
    console.log('Fetching parsed data for queries:', queries);
    setLoading(true);
    try {
      const response = await axios.post(
        'http://localhost:5000/parse-queries',
        { queries },
        {
          headers: {
            'Content-Type': 'application/json',
          },
        }
      );
      console.log('Received response:', response.data);

      // Organize parsed data per query with ID as count
      const organizedData = {};
      Object.keys(response.data).forEach((key) => {
        const item = response.data[key];
        const { query, goal_general, goal_specific, goals } = item;
        if (!organizedData[query]) {
          organizedData[query] = {
            id: queries.indexOf(query) + 1, // Assign ID based on query count
            goal_general: goal_general,
            goal_specific: goal_specific,
            clauses: [],
          };
        }
        // Iterate over the goals object
        Object.keys(goals).forEach((goalKey) => {
          const goalItem = goals[goalKey];
          organizedData[query].clauses.push({
            clause: goalItem.type,
            goal: goalItem.content,
          });
        });
      });
      setParsedData(organizedData);

      setSnackbar({
        open: true,
        message: 'File processed successfully!',
        severity: 'success',
      });
    } catch (error) {
      console.error('Error fetching parsed data:', error);
      setSnackbar({
        open: true,
        message: 'An error occurred while processing the file. Please try again.',
        severity: 'error',
      });
    } finally {
      setLoading(false);
    }
  };

  // Handler to close the Snackbar
  const handleCloseSnackbar = () => {
    setSnackbar({ ...snackbar, open: false });
  };

  return (
    <Container
      sx={{
        p: 4,
        maxWidth: 1200,
        mx: 'auto',
        textAlign: 'center',
      }}
    >
      <Typography variant="h4" gutterBottom>
        Query Overview
      </Typography>
      <Typography variant="subtitle1" gutterBottom>
        Upload a CSV file containing SQL reference queries to process.
      </Typography>

      {/* File Upload Section using FileUploadBox */}
      <Box sx={{ mt: 4 }}>
        <FileUploadBox
          label="Reference Queries"
          fileType="referenceQueries"
          file={csvFile}
          onFileDrop={handleFileDrop}
          onRemoveFile={handleRemoveFile}
          error={null} // Replace with any error message if needed
          accept={{
            'text/csv': ['.csv'],
          }}
        />
      </Box>

      {/* Action Button */}
      <Box sx={{ mt: 4 }}>
        <Button
          variant="contained"
          color="primary"
          onClick={handleProcessFile}
          disabled={!csvFile || loading}
          startIcon={<UploadFileIcon />}
          sx={{ py: 1, px: 3 }}
        >
          Process Query
        </Button>
      </Box>

      {/* Loading Indicator */}
      {loading && (
        <Box sx={{ mt: 4 }}>
          <Typography variant="subtitle1">Processing...</Typography>
          <LinearProgress sx={{ mt: 2 }} />
        </Box>
      )}

      {/* Display Parsed Data */}
      {Object.keys(parsedData).length > 0 && (
        <Container maxWidth="lg">
          <Box sx={{ mt: 4 }}>
            {queries.map((query, index) => {
              const data = parsedData[query];
              if (!data) {
                // Data not available yet, skip rendering this query or show a placeholder
                return null;
              }
              return (
                <Box key={index} sx={{ mb: 6 }}>
                  {/* Query */}
                  <Grid container spacing={3} sx={{ mt: 2 }}>
                    <Grid item xs={12} sm={12}>
                      <Card sx={{ display: 'flex', alignItems: 'center', p: 2 }} elevation={1}>
                        <SearchIcon sx={{ fontSize: 50, color: theme.palette.info.main, mr: 2 }} />
                        <Box sx={{ textAlign: 'left' }}>
                          <Typography variant="h6" component="div">
                            Query
                          </Typography>
                          <Typography color="text.secondary">
                            {data.id}: {query}
                          </Typography>
                        </Box>
                      </Card>
                    </Grid>
                  </Grid>

                  {/* Goals */}
                  <Grid container spacing={3} sx={{ mt: 2 }}>
                    {/* General goal */}
                    <Grid item xs={12} sm={6}>
                      <Card sx={{ display: 'flex', alignItems: 'center', p: 2 }} elevation={1}>
                        <FlagIcon sx={{ fontSize: 50, color: theme.palette.info.main, mr: 2 }} />
                        <Box sx={{ textAlign: 'left' }}>
                          <Typography variant="h6" component="div">
                            General goal
                          </Typography>
                          <Typography color="text.secondary">
                            {data.goal_general}
                          </Typography>
                        </Box>
                      </Card>
                    </Grid>
                    {/* Specific goal */}
                    <Grid item xs={12} sm={6}>
                      <Card sx={{ display: 'flex', alignItems: 'center', p: 2 }} elevation={1}>
                        <AdjustIcon sx={{ fontSize: 50, color: theme.palette.error.main, mr: 2 }} />
                        <Box sx={{ textAlign: 'left' }}>
                          <Typography variant="h6" component="div">
                            Specific goal
                          </Typography>
                          <Typography color="text.secondary">
                            {data.goal_specific}
                          </Typography>
                        </Box>
                      </Card>
                    </Grid>
                  </Grid>

                  {/* Clauses Table */}
                  <TableContainer component={Paper} elevation={3} sx={{ mt: 2 }}>
                    <Table aria-label={`Parsed data for query ${data.id}`}>
                      <TableHead>
                        <TableRow sx={{ backgroundColor: theme.palette.primary.light }}>
                          <TableCell
                            sx={{ color: 'white', fontWeight: 'bold', width: '10%' }}
                          >
                            ID
                          </TableCell>
                          <TableCell
                            sx={{ color: 'white', fontWeight: 'bold', width: '45%' }}
                          >
                            Clause
                          </TableCell>
                          <TableCell
                            sx={{ color: 'white', fontWeight: 'bold', width: '45%'  }}
                          >
                            Goal
                          </TableCell>
                        </TableRow>
                      </TableHead>
                      <TableBody>
                        {data.clauses.map((row, rowIndex) => (
                          
                          <TableRow key={rowIndex} sx={{
                            '&:nth-of-type(odd)': {
                              backgroundColor: theme.palette.action.hover,
                            },
                          }}>
                            <TableCell>{rowIndex + 1}</TableCell>
                            <TableCell>{row.clause}</TableCell>
                            <TableCell>{row.goal}</TableCell>
                          </TableRow>
                        ))}
                      </TableBody>
                    </Table>
                  </TableContainer>
                </Box>
              );
            })}
          </Box>
        </Container>
      )}

      {/* Snackbar for Notifications */}
      <Snackbar
        open={snackbar.open}
        autoHideDuration={6000}
        onClose={handleCloseSnackbar}
        anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}
      >
        <Alert onClose={handleCloseSnackbar} severity={snackbar.severity} sx={{ width: '100%' }}>
          {snackbar.message}
        </Alert>
      </Snackbar>
    </Container>
  );
}

export default QueryOverview;
