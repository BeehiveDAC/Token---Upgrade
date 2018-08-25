ACTION: addstake

PARAMETERS: ( _stake_account ) is a type of  eosio accountname
            (_stake_period) is a duration the user wishes to stake the tokens for (weekly, monthly or quarterly)
            (_staked) is a type of eosio asset and is the amount of EDNA tokens the users wishes to stake

INTENT: The intent of {{ addstake }} is to allow a user to hold EDNA tokens in the contract and earn bonus EDNA tokens for doing so. Longer durations are intended to earn a greater percentage of the available bonus funds for that week as are higher stake amounts. Bonuses are calculated weekly, and added to the existing users staked tokens when the staked period has elapsed.    

TERM: This action lasts for the duration of the processing of the contract.
