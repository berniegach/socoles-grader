'use client';
import { Box } from '@mui/material';
import InstructorDashboard from './InstructorDashboard';
import QuestionManager from './QuestionManager';
import BatchGrader from './BatchGrader';
import SubmissionReview from './SubmissionReview';
import ClassRoster from './ClassRoster';
import InstructorSettings from './InstructorSettings';
import AssignmentQuestionManager from './AssignmentQuestionManager';
import DatasetsManager from './DatasetsManager';
import InstructorReviewRequests from './InstructorReviewRequests';


export default function InstructorArea({ active }: { active: string }) {
    return (
        <Box sx={{ display: 'grid', gap: 2 }}>
            {active === 'i-dash' && <InstructorDashboard />}
            {active === 'i-questions' && <QuestionManager />}
            {active === 'i-assignments' && <AssignmentQuestionManager />}
            {active === 'i-datasets' && <DatasetsManager />}
            {active === 'i-batch' && <BatchGrader />}
            {active === 'i-submissions' && <SubmissionReview />}
            {active === 'i-class' && <ClassRoster />}
            {active === 'i-settings' && <InstructorSettings />}
            {active === 'i-reviews' && <InstructorReviewRequests />}
        </Box>
    );
}