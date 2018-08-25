ACTION: addbonus

PARAMETERS: ( _sender ) is an eisio type account_name
            ( _bonus ) is an eosio type asset

INTENT: The intent of {{ addbonus }} is to allow an EOS account to add additional EDNA Tokens to the total payout bonus available for the current weekly payout. This most usually will come from the overflow account, but could come from elsewhere. Adding to the payout bonus is always at the sole discretion of the contract owner and no bonus is ever an obligation of EDNA or the contract owner.     

TERM: This action lasts for the duration of the processing of the contract.
