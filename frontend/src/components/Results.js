// Results.js

import React, { useState } from 'react';
import {
  Box,
  Typography,
  Button,
  Grid,
  TableContainer,
  Table,
  TableHead,
  TableRow,
  TableCell,
  TableBody,
  Container,
  TablePagination,
  Card,
  CardContent,
  CardHeader,
  useTheme,
} from '@mui/material';
import {
  BarChart,
  Bar,
  XAxis,
  YAxis,
  Tooltip,
  PieChart,
  Pie,
  Cell,
  ResponsiveContainer,
  Legend,
  CartesianGrid,
} from 'recharts';
import {
  People as PeopleIcon,
  Assignment as AssignmentIcon,
  Grade as GradeIcon,
  ArrowUpward as ArrowUpwardIcon,
  ArrowDownward as ArrowDownwardIcon,
  Refresh as RefreshIcon,
  ArrowBack as ArrowBackIcon,
  MergeType as MergeIcon,
  Download as DownloadIcon
} from '@mui/icons-material';
import { mergeAndSave } from './merge'; 


function Results({ gradingResults, onBack, onRestart, studentFile }) {
  const theme = useTheme();

  // Compute statistics
  const totalAnswers = gradingResults.length;
  const uniqueAnswers = new Set(gradingResults.map((result) => result.Query)).size;
  const grades = gradingResults.map((result) => parseFloat(result.Grade));
  const averageGrade = (
    grades.reduce((sum, grade) => sum + grade, 0) / totalAnswers
  ).toFixed(2);
  const maxGrade = Math.max(...grades).toFixed(2);
  const minGrade = Math.min(...grades).toFixed(2);

  // Prepare data for charts
  // Grade Distribution Data
  const gradeCounts = {};

  grades.forEach((grade) => {
    const gradeKey = grade.toFixed(2); // Convert to string with two decimal places
    gradeCounts[gradeKey] = (gradeCounts[gradeKey] || 0) + 1;
  });

  // Convert gradeCounts object to an array and sort it
  const gradeDistributionData = Object.keys(gradeCounts)
    .map((grade) => ({
      grade: parseFloat(grade), // Convert back to number for sorting
      count: gradeCounts[grade], // y-axis value
    }))
    .sort((a, b) => a.grade - b.grade); // Sort in ascending order

  // Convert grades back to strings for display
  gradeDistributionData.forEach((item) => {
    item.grade = item.grade.toFixed(2);
  });

  // Colors for the bars
  const COLORS = [
    theme.palette.primary.light,
    theme.palette.secondary.light,
    theme.palette.error.light,
    theme.palette.warning.light,
    theme.palette.info.light,
    theme.palette.success.light,
    '#8e44ad',
    '#e67e22',
    '#2c3e50',
    '#d35400',
  ];

  // Pass/Fail Counts (Assuming 0.6 is passing)
  const passFailCounts = grades.reduce(
    (acc, grade) => {
      if (grade >= 0.6) acc.pass += 1;
      else acc.fail += 1;
      return acc;
    },
    { pass: 0, fail: 0 }
  );

  const passFailData = [
    { name: 'Pass', value: passFailCounts.pass },
    { name: 'Fail', value: passFailCounts.fail },
  ];

  // Common Incorrect Answers
  const incorrectAnswers = gradingResults
    .filter((result) => parseFloat(result.Grade) < 1.0)
    .map((result) => result.Query);

  const incorrectAnswersCount = {};
  incorrectAnswers.forEach((answer) => {
    incorrectAnswersCount[answer] = (incorrectAnswersCount[answer] || 0) + 1;
  });

  const commonIncorrectAnswers = Object.keys(incorrectAnswersCount)
    .map((answer) => ({
      answer,
      count: incorrectAnswersCount[answer],
    }))
    .sort((a, b) => b.count - a.count)
    .slice(0, 5); // Top 5 common incorrect answers

  // Pagination state for the table
  const [page, setPage] = useState(0); // Current page number
  const [rowsPerPage, setRowsPerPage] = useState(5); // Rows per page options

  // Event handlers for pagination
  const handleChangePage = (event, newPage) => {
    setPage(newPage);
  };

  const handleChangeRowsPerPage = (event) => {
    setRowsPerPage(parseInt(event.target.value, 10));
    setPage(0); // Reset to first page
  };

  // Slice the grading results to display only the current page
  const paginatedResults = gradingResults.slice(
    page * rowsPerPage,
    page * rowsPerPage + rowsPerPage
  );

  const handleMergeAndSave = () => {
    mergeAndSave(studentFile, gradingResults);
  };

// Function to handle CSV download
const handleDownloadCSV = () => {
  if (!gradingResults || gradingResults.length === 0) {
    alert('No data to download');
    return;
  }

  // Define headers in the desired order
  const headers = [
    'Org Defined ID',
    'Attempt #',
    'Q #',
    'Query',
    'Grade',
    'Out Of',
    'Feedback',
  ];

  // Create CSV string
  const csvRows = [];
  csvRows.push(headers.join(',')); // headers row

  gradingResults.forEach((result) => {
    const values = headers.map((header) => {
      const escaped = (`${result[header]}`).replace(/"/g, '""'); // Escape double quotes
      return `"${escaped}"`;
    });
    csvRows.push(values.join(','));
  });

  const csvString = csvRows.join('\n');

  // Create Blob and trigger download
  const blob = new Blob([csvString], { type: 'text/csv;charset=utf-8;' });
  const url = URL.createObjectURL(blob);

  const link = document.createElement('a');
  link.href = url;
  link.setAttribute('download', 'grading_results.csv');
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
};



  return (
    <Container maxWidth="lg">
      <Box sx={{ mt: 4 }}>
        {/* Summary Statistics */}
        <Grid container spacing={3}>
          {/* Total Answers */}
          <Grid item xs={12} sm={6} md={4}>
            <Card
              sx={{ display: 'flex', alignItems: 'center', p: 2 }}
              elevation={1}
            >
              <PeopleIcon
                sx={{ fontSize: 50, color: theme.palette.primary.main, mr: 2 }}
              />
              <Box>
                <Typography variant="h5" component="div">
                  {totalAnswers}
                </Typography>
                <Typography color="text.secondary">Total Answers</Typography>
              </Box>
            </Card>
          </Grid>
          {/* Unique Answers */}
          <Grid item xs={12} sm={6} md={4}>
            <Card
              sx={{ display: 'flex', alignItems: 'center', p: 2 }}
              elevation={1}
            >
              <AssignmentIcon
                sx={{ fontSize: 50, color: theme.palette.secondary.main, mr: 2 }}
              />
              <Box>
                <Typography variant="h5" component="div">
                  {uniqueAnswers}
                </Typography>
                <Typography color="text.secondary">Unique Answers</Typography>
              </Box>
            </Card>
          </Grid>
          {/* Average Grade */}
          <Grid item xs={12} sm={6} md={4}>
            <Card
              sx={{ display: 'flex', alignItems: 'center', p: 2 }}
              elevation={1}
            >
              <GradeIcon
                sx={{ fontSize: 50, color: theme.palette.success.main, mr: 2 }}
              />
              <Box>
                <Typography variant="h5" component="div">
                  {averageGrade}
                </Typography>
                <Typography color="text.secondary">Average Grade</Typography>
              </Box>
            </Card>
          </Grid>
        </Grid>

        {/* High and Low Grades */}
        <Grid container spacing={3} sx={{ mt: 2 }}>
          {/* Highest Grade */}
          <Grid item xs={12} sm={6}>
            <Card
              sx={{ display: 'flex', alignItems: 'center', p: 2 }}
              elevation={1}
            >
              <ArrowUpwardIcon
                sx={{ fontSize: 50, color: theme.palette.info.main, mr: 2 }}
              />
              <Box>
                <Typography variant="h5" component="div">
                  {maxGrade}
                </Typography>
                <Typography color="text.secondary">Highest Grade</Typography>
              </Box>
            </Card>
          </Grid>
          {/* Lowest Grade */}
          <Grid item xs={12} sm={6}>
            <Card
              sx={{ display: 'flex', alignItems: 'center', p: 2 }}
              elevation={1}
            >
              <ArrowDownwardIcon
                sx={{ fontSize: 50, color: theme.palette.error.main, mr: 2 }}
              />
              <Box>
                <Typography variant="h5" component="div">
                  {minGrade}
                </Typography>
                <Typography color="text.secondary">Lowest Grade</Typography>
              </Box>
            </Card>
          </Grid>
        </Grid>

        {/* Visualizations */}
        <Grid container spacing={3} sx={{ mt: 2 }}>
          {/* Bar Chart for Grade Distribution */}
          <Grid item xs={12} md={6}>
            <Card elevation={1}>
              <CardHeader title="Grade Distribution" />
              <CardContent>
                <ResponsiveContainer width="100%" height={300}>
                  <BarChart data={gradeDistributionData}>
                    <CartesianGrid
                      strokeDasharray="3 3"
                      horizontal={true}
                      vertical={false}
                    />
                    <XAxis
                      dataKey="grade"
                      label={{
                        value: 'Grade',
                        position: 'insideBottom',
                        offset: -5,
                      }}
                    />
                    <YAxis
                      label={{
                        value: 'Count',
                        angle: -90,
                        position: 'insideLeft',
                      }}
                    />
                    <Tooltip />
                    <Bar dataKey="count">
                      {gradeDistributionData.map((entry, index) => (
                        <Cell
                          key={`cell-${index}`}
                          fill={COLORS[index % COLORS.length]}
                        />
                      ))}
                    </Bar>
                  </BarChart>
                </ResponsiveContainer>
              </CardContent>
            </Card>
          </Grid>
          {/* Pie Chart for Pass/Fail Rate */}
          <Grid item xs={12} md={6}>
            <Card elevation={1}>
              <CardHeader title="Pass/Fail Rate" />
              <CardContent>
                <ResponsiveContainer width="100%" height={300}>
                  <PieChart>
                    <Pie
                      data={passFailData}
                      dataKey="value"
                      nameKey="name"
                      cx="50%"
                      cy="50%"
                      outerRadius={80}
                      label
                    >
                      {passFailData.map((entry, index) => (
                        <Cell
                          key={`cell-${index}`}
                          fill={
                            entry.name === 'Pass'
                              ? theme.palette.success.light
                              : theme.palette.error.light
                          }
                        />
                      ))}
                    </Pie>
                    <Tooltip />
                    <Legend />
                  </PieChart>
                </ResponsiveContainer>
              </CardContent>
            </Card>
          </Grid>
        </Grid>

        {/* Common Incorrect Answers */}
        <Box sx={{ mt: 4 }}>
          <Card elevation={1}>
            <CardHeader title="Common Incorrect Answers" />
            <CardContent>
              {commonIncorrectAnswers && commonIncorrectAnswers.length > 0 ? (
                <TableContainer>
                  <Table>
                    <TableHead>
                      <TableRow sx={{ backgroundColor: theme.palette.primary.light }}>
                        <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                          Count
                        </TableCell>
                        <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                          Query
                        </TableCell>
                      </TableRow>
                    </TableHead>
                    <TableBody>
                      {commonIncorrectAnswers.map((item, index) => (
                        <TableRow key={index}>
                          <TableCell>{item.count}</TableCell>
                          <TableCell
                            sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}
                          >
                            {item.answer}
                          </TableCell>
                        </TableRow>
                      ))}
                    </TableBody>
                  </Table>
                </TableContainer>
              ) : (
                <Typography>No common incorrect answers available.</Typography>
              )}
            </CardContent>
          </Card>
        </Box>

        {/* Grading Results Table */}
        <Box sx={{ mt: 4 }}>
          <Card elevation={1}>
            <CardHeader title="Detailed Grading Results"  action={
                <Box sx={{ display: 'flex', gap: 1 }}>
                  <Button
                    variant="contained"
                    color="success"
                    onClick={handleMergeAndSave}
                    startIcon={<MergeIcon />}
                  >
                    Merge and Save Results
                  </Button>
                  <Button
                    variant="contained"
                    color="primary"
                    onClick={handleDownloadCSV}
                    startIcon={<DownloadIcon />}
                  >
                    Download CSV
                  </Button>
                </Box>
              } />
            <CardContent>
              {gradingResults && gradingResults.length > 0 ? (
                <>
                  <TableContainer>
                    <Table aria-label="grading results table">
                      <TableHead>
                        <TableRow sx={{ backgroundColor: theme.palette.primary.light }}>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Org Defined ID
                          </TableCell>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Attempt #
                          </TableCell>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Q #
                          </TableCell>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Query
                          </TableCell>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Grade
                          </TableCell>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Out Of
                          </TableCell>
                          <TableCell sx={{ color: 'white', fontWeight: 'bold' }}>
                            Feedback
                          </TableCell>
                        </TableRow>
                      </TableHead>
                      <TableBody>
                        {paginatedResults.map((result, index) => (
                          <TableRow
                            key={index}
                            sx={{
                              '&:nth-of-type(odd)': {
                                backgroundColor: theme.palette.action.hover,
                              },
                            }}
                          >
                            <TableCell>{result['Org Defined ID']}</TableCell>
                            <TableCell>{result['Attempt #']}</TableCell>
                            <TableCell>{result['Q #']}</TableCell>
                            <TableCell
                              sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}
                            >
                              {result['Query']}
                            </TableCell>
                            <TableCell>{result['Grade']}</TableCell>
                            <TableCell>{result['Out Of']}</TableCell>
                            <TableCell
                              sx={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}
                            >
                              {result['Feedback']}
                            </TableCell>
                          </TableRow>
                        ))}
                      </TableBody>
                    </Table>
                  </TableContainer>
                  {/* Table Pagination */}
                  <TablePagination
                    component="div"
                    count={gradingResults.length}
                    page={page}
                    onPageChange={handleChangePage}
                    rowsPerPage={rowsPerPage}
                    onRowsPerPageChange={handleChangeRowsPerPage}
                    rowsPerPageOptions={[5, 10, 25, { label: 'All', value: gradingResults.length }]}
                    labelRowsPerPage="Rows per page:"
                  />
                </>
              ) : (
                <Typography>No grading results available.</Typography>
              )}
            </CardContent>
          </Card>
        </Box>

        {/* Navigation Buttons */}
        <Box
          sx={{
            mt: 4,
            display: 'flex',
            justifyContent: { xs: 'center', sm: 'flex-start' },
            flexWrap: 'wrap',
            gap: 2,
          }}
        >
          <Button
            variant="outlined"
            color="secondary"
            onClick={onBack}
            startIcon={<ArrowBackIcon />}
          >
            Back
          </Button>
          <Button
            variant="contained"
            color="primary"
            onClick={onRestart}
            startIcon={<RefreshIcon />}
          >
            New Submission
          </Button>        
        </Box>
      </Box>
    </Container>
  );
}

export default Results;
