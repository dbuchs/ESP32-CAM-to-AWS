// Import the required AWS SDK modules
import { S3Client, PutObjectCommand } from '@aws-sdk/client-s3';

// Create an S3 client
const s3Client = new S3Client({ region: 'us-east-2' });

// Lambda function handler
export const handler = async (event, context) => {
    // Extract the base64 encoded file data from the event
    const base64FileData = event.image;

    // Decode the base64 data to get the binary file data
    const binaryFileData = Buffer.from(base64FileData, 'base64');

    // Specify your S3 bucket and object key
    const bucketName = 'buchs-cameraupload';
    const currentTimestamp = new Date().toISOString();
    const objectKey = `files/${currentTimestamp}.jpg`;

    // Create an S3 PutObjectCommand
    const s3Params = {
        Bucket: bucketName,
        Key: objectKey,
        Body: binaryFileData,
   //     ACL: 'public-read' // Optional: Set the access control list as needed
    };
    const putObjectCommand = new PutObjectCommand(s3Params);

    // Upload the binary file data to S3
    try {
        await s3Client.send(putObjectCommand);
        console.log('File uploaded to S3 successfully');
    } catch (err) {
        console.error('Error uploading file to S3:', err);
    }
};
