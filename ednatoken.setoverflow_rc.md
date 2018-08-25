ACTION: setoverflow

PARAMETERS: (_overflow) is a type of eosio accountname

INTENT: The intent of {{ _overflow }} is to provide a storage account for tokens that were available to be paid out as a bonus, but not paid because the total staked amount in the contract was less than 100% of the total supply of EDNA Tokens. This overflow storage can be recycled into the bonus payout in following periods using the addbonus action.

TERM: This action lasts for the duration of the processing of the contract.
