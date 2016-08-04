
## Envelope structure

- metamodel: STRING
- model: STRING
- type: STRING
- payload: JSON subpart

## Payload structure
Here all msgs are listed with their type and payload structure.


### Type: RSGUpdate_local|RSGUpdate_global|RSGUpdate_both
This is a message containing an update to the graph of the RSG. The RSGUpdate_local is used by software components to update their local RSG. The RSGUpdate_global is used by the RSG itself to update the RSGs on other agents. RSGUpdate_both could be used by software components to do both at the same time, but is only used for debugging purposes.

For the definition of the payload, please refer to the the RSGUpdate message schemas in its repository.

### Type: RSGQuery
This message type is used by local components to ask queries to the RSG. For the definition of the payload, please refer to the the message schema in its repository. For the returned result see RSGQueryResult below or the example code.

### Type: RSGFunctionBlock
This message type is used by local components to trigger the execution of a function block in the RSG. For the definition of the payload, please refer to the the message schema in its repository. For the returned result see RSGFunctionBlockResult below or the example code.

### Type: RSGUpdateResult|RSGQueryResult|RSGFunctionBlockResult
For every query to the RSG, there will be a broadcasted reply. In order to use it, filter for the corresponding result type and then look in the payload whether the result has the queryId that you gave your query.

For a detailed definition of the payload, please refer to the the message schemas in the repository of the RSG.

## Example code
TODO: write an example component that 