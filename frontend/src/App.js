import React from 'react';
import { BrowserRouter as Router, Routes, Route, Link } from 'react-router-dom';
import {
  AppBar,
  Toolbar,
  Button,
  Stack,
  Typography,
  Box,
} from '@mui/material';
import HomeIcon from '@mui/icons-material/Home';
import AddTaskIcon from '@mui/icons-material/AddTask';
import AssessmentIcon from '@mui/icons-material/Assessment'; // Import an icon for QueryOverview
import GradingIcon from '@mui/icons-material/Grading';
import Home from './components/Home';
import Grade from './components/Grade';
import QueryOverview from './components/QueryOverview'; // Import QueryOverview component

function App() {
  return (
    <Router>
      <AppBar position="static" sx={{ backgroundColor: '#1976d2' }}>
        <Toolbar>
          {/* Brand Logo or Name */}
          <Typography variant="h6" sx={{ flexGrow: 1 }}>
            <Link to="/" style={{ textDecoration: 'none', color: 'inherit' }}>
              Socoles
            </Link>
          </Typography>

          {/* Navigation Buttons */}
          <Stack direction="row" spacing={2}>
            <Button
              color="inherit"
              component={Link}
              to="/"
              startIcon={<HomeIcon />}
              sx={{ textTransform: 'none' }}
            >
              Home
            </Button>
            <Button
              color="inherit"
              component={Link}
              to="/new-task"
              startIcon={<GradingIcon />}
              sx={{ textTransform: 'none' }}
            >
              Grade
            </Button>
            <Button
              color="inherit"
              component={Link}
              to="/query-overview"
              startIcon={<AssessmentIcon />}
              sx={{ textTransform: 'none' }}
            >
              Query Overview
            </Button>
          </Stack>
        </Toolbar>
      </AppBar>

      {/* Routes */}
      <Box sx={{ mt: 4 }}>
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/new-task" element={<Grade />} />
          <Route path="/query-overview" element={<QueryOverview />} />
          {/* Add more routes as needed */}
        </Routes>
      </Box>
    </Router>
  );
}

export default App;
