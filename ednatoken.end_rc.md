ACTION: end

PARAMETERS: ( none )

INTENT: The intent of {{ end }} is first tun off the staking function to disallow new stakes, and then to return all tokens stored on the staking table to the accounts they came from. This effectively "ends" the staking offering, but does not disable the contract. It could be restarted at a later date using the run action with the (true) parameter.

TERM: This action lasts for the duration of the processing of the contract.
