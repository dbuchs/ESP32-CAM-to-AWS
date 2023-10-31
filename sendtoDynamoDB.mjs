import { DynamoDBClient } from "@aws-sdk/client-dynamodb";
import {
  DynamoDBDocumentClient,
  ScanCommand,
  PutCommand,
  GetCommand,
  DeleteCommand,
} from "@aws-sdk/lib-dynamodb";

const client = new DynamoDBClient({});

const dynamo = DynamoDBDocumentClient.from(client);

const tableName = "snslambda";

export const handler = async (event, context) => {
  console.log('got message\n');
  console.log(event);
  console.log('\n');
  
  let body;
  let statusCode = 200;
  const headers = {
    "Content-Type": "application/json",
  };
  const currentTimestamp = new Date().toISOString(); // ISO 8601 format

  try {
    console.log("in try\n");
    console.log("in put\n");
    console.log(event);
    //let requestJSON = JSON.parse(event.body);
    await dynamo.send(
      new PutCommand({
        TableName: tableName,
          Item: {
            TestItem: "testitem",
            SnsTopicArn: "esp32/pub",
            SnsPublishTime: currentTimestamp,
            Event: event,
              // SnsMessageId: event.Records[0].Sns.MessageId,
              // SnsPublishTime: event.Records[0].Sns.Timestamp,
              // SnsTopicArn: event.Records[0].Sns.TopicArn,
            },
          })
        );
        console.log("made PutCommand");
        body = `Put item`;
  } catch (err) {
    statusCode = 400;
    body = err.message;
  } finally {
    console.log(body);
    body = JSON.stringify(body);
  }

  return {
    statusCode,
    body,
    headers,
  };
};
