// merge.js

import Papa from 'papaparse';
import { saveAs } from 'file-saver';

/**
 * Merges grading results into the original student CSV file and prompts the user to save the updated file.
 *
 * @param {File} studentFile - The original CSV file uploaded by the user.
 * @param {Array} gradingResults - The array of grading results.
 */
export function mergeAndSave(studentFile, gradingResults) {
  if (!studentFile) {
    alert('Original CSV file is not available.');
    return;
  }

  Papa.parse(studentFile, {
    header: true,
    skipEmptyLines: true,
    dynamicTyping: true, // Ensure numbers are parsed correctly
    complete: (results) => {
      const originalData = results.data;

      // Define columns used for merging
      const mergeColumns = ['Org Defined ID', 'Attempt #', 'Q #'];
      
      // Define which columns to update and their corresponding mappings
      const columnMapping = {
        'Grade': 'Score', // Map 'Grade' from gradingResults to 'Score' in originalData
        'Out Of': 'Out Of', // Assuming 'Out Of' has the same name
        'Feedback': 'Feedback' // Assuming 'Feedback' has the same name
      };

      // Function to normalize values for consistent key matching
      function normalizeValue(value) {
        if (typeof value === 'string') {
          return value.trim().toLowerCase();
        } else if (typeof value === 'number') {
          return value.toString();
        }
        return '';
      }

      // Create a map for quick lookup of grading results
      const gradingResultsMap = {};
      gradingResults.forEach((result) => {
        const key = mergeColumns
          .map((col) => normalizeValue(result[col]))
          .join('|');
        gradingResultsMap[key] = result;
      });

      // Update the original data with grading results
      originalData.forEach((row) => {
        const key = mergeColumns
          .map((col) => normalizeValue(row[col]))
          .join('|');

        if (gradingResultsMap[key]) {
          // Iterate over the column mappings
          for (const [sourceCol, targetCol] of Object.entries(columnMapping)) {
            if (gradingResultsMap[key][sourceCol] !== undefined) {
              let value = gradingResultsMap[key][sourceCol];

              // Specific formatting for numerical fields
              if ((targetCol === 'Score' || targetCol === 'Out Of') && typeof value === 'number') {
                // Format to 2 decimal places
                value = value.toFixed(2);
              }

              // Overwrite the target column in the original data
              row[targetCol] = value;
            }
          }
        }
      });

      // Optional: Convert all numerical fields to strings to preserve formatting
      originalData.forEach((row) => {
        for (let key in row) {
          if (typeof row[key] === 'number' && !Number.isInteger(row[key])) {
            row[key] = row[key].toFixed(2); // Ensure 2 decimal places
          }
        }
      });

      // Convert the updated data back to CSV
      const csv = Papa.unparse(originalData, {
        quotes: true, // Enclose all fields in quotes for consistency
      });

      // Prepare the CSV file for download
      const fileName = `updated_${studentFile.name}`;
      const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
      saveAs(blob, fileName);
    },
    error: (error) => {
      console.error('Error parsing original CSV file:', error);
      alert('An error occurred while parsing the original CSV file.');
    },
  });
}
