// src/components/GradingOptions.js

import React from 'react';
import {
  Box,
  Container,
  Grid,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Switch,
  FormControlLabel,
  TextField,
} from '@mui/material';

function GradingOptions({ options, handleOptionChange, questionNumbers, selectedQuestionNumber, setSelectedQuestionNumber, }) {
  return (
    <Container maxWidth="md" sx={{ mt: 6 }}>
      <Grid container spacing={4}>
        {/* Question Number */}
        <Grid item xs={12} sm={6}>
          <FormControl fullWidth>
            <InputLabel id="question-number-label">Select Question Number</InputLabel>
            <Select
              labelId="question-number-label"
              id="question-number-select"
              value={selectedQuestionNumber}
              label="Select Question Number"
              onChange={(e) => setSelectedQuestionNumber(e.target.value)}
            >
              {questionNumbers.map((qNum) => (
                <MenuItem key={qNum} value={qNum}>
                  {qNum}
                </MenuItem>
              ))}
            </Select>
          </FormControl>
        </Grid>

        {/* Syntax Sensitivity */}
        <Grid item xs={12} sm={6}>
          <FormControl fullWidth>
            <InputLabel id="syntax-sensitivity-label">Syntax Sensitivity</InputLabel>
            <Select
              labelId="syntax-sensitivity-label"
              id="syntax-sensitivity"
              value={options.syntaxSensitivity}
              label="Syntax Sensitivity"
              onChange={handleOptionChange('syntaxSensitivity')}
            >
              <MenuItem value="0 Absent">Absent</MenuItem>
              <MenuItem value="2 Levels">2 Levels</MenuItem>
              <MenuItem value="3 Levels">3 Levels</MenuItem>
            </Select>
          </FormControl>
        </Grid>

        {/* Semantics Sensitivity */}
        <Grid item xs={12} sm={6}>
          <FormControl fullWidth>
            <InputLabel id="semantics-sensitivity-label">Semantics Sensitivity</InputLabel>
            <Select
              labelId="semantics-sensitivity-label"
              id="semantics-sensitivity"
              value={options.semanticsSensitivity}
              label="Semantics Sensitivity"
              onChange={handleOptionChange('semanticsSensitivity')}
            >
              <MenuItem value="0 Absent">Absent</MenuItem>
              <MenuItem value="2 Levels">2 Levels</MenuItem>
              <MenuItem value="3 Levels">3 Levels</MenuItem>
              <MenuItem value="8 Levels">8 Levels</MenuItem>
            </Select>
          </FormControl>
        </Grid>

        {/* Results Sensitivity */}
        <Grid item xs={12} sm={6}>
          <FormControl fullWidth>
            <InputLabel id="results-sensitivity-label">Results Sensitivity</InputLabel>
            <Select
              labelId="results-sensitivity-label"
              id="results-sensitivity"
              value={options.resultsSensitivity}
              label="Results Sensitivity"
              onChange={handleOptionChange('resultsSensitivity')}
            >
              <MenuItem value="0 Absent">Absent</MenuItem>
              <MenuItem value="2 Levels">2 Levels</MenuItem>
              <MenuItem value="3 Levels">3 Levels</MenuItem>
            </Select>
          </FormControl>
        </Grid>

        {/* Evaluation Priority */}
        <Grid item xs={12} sm={6}>
          <FormControl fullWidth>
            <InputLabel id="evaluation-priority-label">Evaluation Priority</InputLabel>
            <Select
              labelId="evaluation-priority-label"
              id="evaluation-priority"
              value={options.evaluationPriority}
              label="Evaluation Priority"
              onChange={handleOptionChange('evaluationPriority')}
            >
              <MenuItem value="1 - Syntax, Semantics, Results">
                1 - Syntax, Semantics, Results
              </MenuItem>
              <MenuItem value="2 - Semantics, Syntax, Results">
                2 - Semantics, Syntax, Results
              </MenuItem>
              <MenuItem value="3 - Results, Semantics, Syntax">
                3 - Results, Semantics, Syntax
              </MenuItem>
              <MenuItem value="4 - Syntax, Results, Semantics">
                4 - Syntax, Results, Semantics
              </MenuItem>
              <MenuItem value="5 - Semantics, Results, Syntax">
                5 - Semantics, Results, Syntax
              </MenuItem>
              <MenuItem value="6 - Results, Syntax, Semantics">
                6 - Results, Syntax, Semantics
              </MenuItem>
            </Select>
          </FormControl>
        </Grid>


        {/* Text Edit Distance Threshold */}
        <Grid item xs={12} sm={6}>
          <TextField
            label="Text Edit Distance Threshold"
            type="number"
            inputProps={{ min: 0 }}
            fullWidth
            value={options.textEditDistance}
            onChange={handleOptionChange('textEditDistance')}
            variant="outlined"
            size="small"
          />
        </Grid>

        {/* Tree Edit Distance Threshold */}
        <Grid item xs={12} sm={6}>
          <TextField
            label="Tree Edit Distance Threshold"
            type="number"
            inputProps={{ min: 0 }}
            fullWidth
            value={options.treeEditDistance}
            onChange={handleOptionChange('treeEditDistance')}
            variant="outlined"
            size="small"
          />
        </Grid>

        {/* Check Order */}
        <Grid item xs={12} sm={6}>
          <FormControlLabel
            control={
              <Switch
                checked={options.checkOrder}
                onChange={handleOptionChange('checkOrder')}
                name="checkOrder"
                color="primary"
              />
            }
            label="Check Order"
          />
        </Grid>


        {/* Auto DB */}
        <Grid item xs={12} sm={6}>
          <FormControlLabel
            control={
              <Switch
                checked={options.autoDB}
                onChange={handleOptionChange('autoDB')}
                name="autoDB"
                color="primary"
              />
            }
            label="Auto DB"
          />
        </Grid>

        {/* Use PostGreSQL DBMS */}
        <Grid item xs={12} sm={6}>
          <FormControlLabel
            control={
              <Switch
                checked={options.use_postgresql}
                onChange={handleOptionChange('use_postgresql')}
                name="use_postgresql"
                color="primary"
              />
            }
            label="Use PostgreSQL"
          />
        </Grid>

        {/* Conditional Rendering for Auto DB Options */}
        {options.autoDB && (
          <>
            {/* DB Name */}
            <Grid item xs={12} sm={6}>
              <TextField
                label="DB Name"
                fullWidth
                value={options.dbName}
                onChange={handleOptionChange('dbName')}
                variant="outlined"
                size="small"
              />
            </Grid>

            {/* Number of DBs */}
            <Grid item xs={12} sm={6}>
              <TextField
                label="Number of DBs"
                type="number"
                inputProps={{ min: 1 }}
                fullWidth
                value={options.numberOfDBs}
                onChange={handleOptionChange('numberOfDBs')}
                variant="outlined"
                size="small"
              />
            </Grid>
          </>
        )}
      </Grid>
    </Container>
  );
}

export default GradingOptions;
