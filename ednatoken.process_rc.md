ACTION: process

PARAMETERS: ( _pay_indicator ) is integer where a value of 0 indicates run the action but to not transfer tokens (for testing), and a non-zero value transfers the tokens.
            ( _bonus ) is an eosio type asset

INTENT: The intent of {{ process }} is to place into the stake table additional tokens earned by users staking their EDNA tokens. These additional tokens called bonus tokens are distributed to according to the following formula:

    Weekly Bonus = 2,000,000.0000 EDNA
    Total Tokens = 1,000,000,000.0000 EDNA
    Staked Tokens = the total of all tokens present on the stake table
    Percentage Staked = Staked Tokens divided by Total Tokens
    Base Payout = Weekly Bonus times Percentage Staked
    Total Payout = Base Payout plus any additional tokens sent using the addbonus actions

    Each Stake entry in the table will receive a multiplier based on the length of the stake made as follows:
    1) Weekly = times 1
    2) Monthly = times 1.5
    3) Quarterly = times 2.0

    The process then adds all stake amounts including their associated multipliers to arrive at a total number of "shares earned".
    Shares are not a representation of ownership, simply a term to represent percentage of bonus each account is awarded for that week as in "share of the total".

    Single Share value = Total Payout divided by the total shares earned expressed in EDNA's
    The Single Share value times the number of shares in each staked row on the staked table is added to the table in one of three ways:
    1) if the table row is staked for a weekly term, the tokens are added to the existing stake
    2) if the table row is staked for a monthly or quarterly term and the term is completed, the tokens are added to the existing stake
    3) if the table row is staked for a monthly or quarterly term and that term is not yet elapsed (for example Week #2 of a monthly stake) The tokens are temporarily stored on an escrow field in the stake table row and are added to the existing stake at the end of the term.  



TERM: This action lasts for the duration of the processing of the contract.
