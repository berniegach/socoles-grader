import React from 'react';
import { Box, Typography, Button } from '@mui/material';
import { Link } from 'react-router-dom';
import AddTaskIcon from '@mui/icons-material/AddTask';

function Home() {
  return (
    <Box
      sx={{
        p: 4,
        textAlign: 'center',
      }}
    >
      <Typography variant="h4" gutterBottom>
        Welcome to Socoles
      </Typography>
      <Typography variant="body1" gutterBottom>
        Grade SQL queries efficiently and effectively.
      </Typography>
      <Button
        variant="contained"
        color="primary"
        component={Link}
        to="/new-task"
        startIcon={<AddTaskIcon />}
        sx={{ mt: 4 }}
      >
        Create a New Task
      </Button>
    </Box>
  );
}

export default Home;
