/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "ednatoken.hpp"
#include <math.h>

void ednatoken::create(account_name issuer,
                       asset maximum_supply)
{
    require_auth(_self);

    auto sym = maximum_supply.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable(_self, sym.name());
    auto existing = statstable.find(sym.name());
    eosio_assert(existing == statstable.end(), "stake with symbol already exists");

    statstable.emplace(_self, [&](auto &s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply = maximum_supply;
        s.issuer = issuer;
    });
}

void ednatoken::issue(account_name to, asset quantity, string memo)
{
    auto sym = quantity.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    auto sym_name = sym.name();
    stats statstable(_self, sym_name);
    auto existing = statstable.find(sym_name);
    eosio_assert(existing != statstable.end(), "stake with symbol does not exist, create stake before issue");
    const auto &st = *existing;

    require_auth(st.issuer);
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");

    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify(st, 0, [&](auto &s) {
        s.supply += quantity;
    });

    add_balance(st.issuer, quantity, st.issuer);

    if (to != st.issuer)
    {
        SEND_INLINE_ACTION(*this, transfer, {st.issuer, N(active)}, {st.issuer, to, quantity, memo});
    }
}

void ednatoken::transfer(account_name from,
                         account_name to,
                         asset quantity,
                         string memo)
{
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    eosio_assert(is_account(to), "to account does not exist");
    auto sym = quantity.symbol.name();
    stats statstable(_self, sym);
    const auto &st = statstable.get(sym);

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    sub_balance(from, quantity);
    add_balance(to, quantity, from);
}

void ednatoken::setoverflow(account_name _overflow)
{
    require_auth(_self);
    config_table c_t(_self, _self);
    auto c_itr = c_t.find(0);
    if (c_itr == c_t.end())
    {
        c_t.emplace(_self, [&](auto &c) {
            c.overflow = _overflow;
        });
    }
    else
    {
        c_t.modify(c_itr, _self, [&](auto &c) {
            c.overflow = _overflow;
        });
    }
}

void ednatoken::addbonus(account_name _sender, asset _bonus)
{
    require_auth(_sender);
    config_table c_t(_self, _self);
    auto c_itr = c_t.find(0);
    if (c_itr == c_t.end())
    {
        c_t.emplace(_self, [&](auto &c) {
            c.bonus = _bonus;
        });
    }
    else
    {
        c_t.modify(c_itr, _self, [&](auto &c) {
              c.bonus += _bonus;
        });
    }
    sub_balance(_sender, _bonus);
}


void ednatoken::setprocess(uint8_t _zero_bonus)
{
    require_auth(_self);
    config_table c_t(_self, _self);
    auto c_itr = c_t.find(0);
    c_t.modify(c_itr, _self, [&](auto &c) {
    if (_zero_bonus == 1 && c_itr->bonus.amount > 0){
      transfer(_self, c_itr->overflow, c.bonus, "reducing bonus --> overflow acount");
      c.bonus -= c.bonus;
    }
    if (c.unclaimed_tokens.amount > 0){
      transfer(_self, c_itr->overflow, c.unclaimed_tokens, "transfering last weeks unclaimed");
      c.unclaimed_tokens -= c.unclaimed_tokens;
      }
    });

    uint64_t total_shares = 0;
    asset total_payout;
    auto total_stake = (c_itr->staked_weekly + c_itr->staked_monthly + c_itr->staked_quarterly);
    total_shares = (WEEK_MULTIPLIERX100 * c_itr->staked_weekly / 10000 / 100);
    total_shares += (MONTH_MULTIPLIERX100 * c_itr->staked_monthly / 10000 / 100);
    total_shares += (QUARTER_MULTIPLIERX100 * c_itr->staked_quarterly / 10000 / 100);

    auto weekly_base = asset{BASE_WEEKLY, string_to_symbol(4, "EXNA")}; // TESTING ONLY Uncomment above to go-live
    auto supply = 1000000000;
    auto perc_stakedx100 = total_stake / supply;
    auto base_payout = perc_stakedx100 * weekly_base.amount / 10000;
    total_payout = asset{static_cast<int64_t>(base_payout + c_itr->bonus.amount), string_to_symbol(4, "EXNA")};
    auto unclaimed_tokens = weekly_base + c_itr->bonus - total_payout;
    auto pay_per_stake = total_payout.amount/(total_stake / 10000 / 100);

    c_t.modify(c_itr, _self, [&](auto &c) {
        c.base_payout = base_payout;
        c.total_shares = total_shares;
        c.unclaimed_tokens = unclaimed_tokens;
    });

    if (total_payout.amount == 0 || total_stake == 0)

    {
        print("Payout: ", total_payout, " Staked: ",total_stake, "% Staked: ", perc_stakedx100, " Nothing to pay.\n");
        return;
    }
    else{
      config_table c_t(_self, _self);
      auto p_itr = c_t.find(0);
      asset print_staked = asset{static_cast<int64_t>(total_stake), string_to_symbol(4, "EXNA")};
      asset print_per_stake = asset{static_cast<int64_t>(pay_per_stake/100), string_to_symbol(4, "EXNA")};
      eosio::print("Total Staked: ", print_staked, " | Bonus: ", c_itr->bonus," | Total Payout: ", total_payout," | Pay/Stake: ", print_per_stake,"\n");
    }
}

//print("Payout per Stake  :", pay_per_stake, " EDNA's\n");
//"Weekly Base: ", weekly_base, "\n"
//  "Supplied Token      : ", supply, "\n"
//"Total Stake         : ", total_stake, "\n"
//"Total Shares        : ", total_shares, "\n"
//"Perc Stakedx100     : ", perc_stakedx100, "\n"
//"Base Payout         : ", base_payout, "\n"
//"Total Payout        : ", total_payout, "\n"
//"Unclaimed Tokens    : ", unclaimed_tokens, "\n"
//);

void ednatoken::claim(account_name _stake_account){

    uint64_t total_shares;
    asset total_payout;
    asset payout;
    // Variables used to keep the config table in sync
    asset add_weekly = asset{static_cast<int64_t>(0.0000), string_to_symbol(4, "EXNA")};
    asset add_monthly = asset{static_cast<int64_t>(0.0000), string_to_symbol(4, "EXNA")};
    asset add_quarterly = asset{static_cast<int64_t>(0.0000), string_to_symbol(4, "EXNA")};
    asset add_escrow =  asset{static_cast<int64_t>(0.0000), string_to_symbol(4, "EXNA")};
    asset rem_escrow =  asset{static_cast<int64_t>(0.0000), string_to_symbol(4, "EXNA")};

    config_table c_t(_self, _self);
    auto c_itr = c_t.find(0);
    total_shares = c_itr->total_shares;
    total_payout = asset{static_cast<int64_t>(c_itr->base_payout + c_itr->bonus.amount), string_to_symbol(4, "EXNA")};

    {
        stake_table s_t(_self, _self);
        auto itr = s_t.find(_stake_account);
        require_auth(itr->stake_account);
        s_t.modify(itr, 0, [&](auto &s) {

        eosio_assert(itr->stake_due <= now(), "You are current on all available claims");

            ///***************          WEEKLY         ****************************//
            if (itr->stake_period == WEEKLY)
            {
                // calc payout
                payout = asset{static_cast<int64_t>((WEEK_MULTIPLIERX100 * itr->staked.amount / 100) / total_shares * total_payout.amount / 10000), string_to_symbol(4, "EXNA")};

                if (itr->stake_due <= now())
                {
                    // pay
                    s.staked += payout;         // increases existing stake
                    add_weekly += payout;
                    sub_balance(_self, payout); // decrement payout from _self
                    //s.stake_due = s.stake_due + (60 * 60 * 24 * 7);
                    s.stake_due = s.stake_due + (60 * 10 );// TESTING ONLY - Uncomment above to go-live
                }
            }
            ///***************          MONTHLY         ****************************//
            else if (itr->stake_period == MONTHLY)
            {
                // calc payout
                payout = asset{static_cast<int64_t>((MONTH_MULTIPLIERX100 * itr->staked.amount / 100) / total_shares * total_payout.amount / 10000), string_to_symbol(4, "EXNA")};

                if (itr->stake_date + (60 * 60 * 24 * 7 * 4) <= now()) { //if the stake_date + 1 month has expired...payout this weeks funds + add_escrow advance both dates
                  s.staked += payout;               // increases existing stake
                  s.staked += s.escrow;             // increases the stake by escrow amount
                  s.escrow -= s.escrow;             // zero the escrow

                  sub_balance(_self, payout + s.escrow);                    // decrement payout from _self
                  //s.stake_due = s.stake_due + (60 * 60 * 24 * 7);         // advance the stake_due by 1 week
                  //s.stake_date = s.stake_date + (60 * 60 * 24 * 7 * 4);   // advance the stake_date by 1 month
                  s.stake_due = s.stake_due + (60 * 10 );                   // TESTING ONLY - Uncomment above to go-live
                  s.stake_date = s.stake_date + (60 * 20 );                 // TESTING ONLY - Uncomment above to go-live
                  add_monthly += payout;                                    // config table book keeping
                  add_monthly += s.escrow;          // config table book keeping
                  rem_escrow += s.escrow;           // config table book keeping
                }
                else if (itr->stake_due + (60 * 60 * 24 * 7) <= now()){ //send payout to escrow and advance the stake_due
                  // add to escrow
                  s.escrow += payout;
                  add_escrow += payout;
                  s.stake_due = s.stake_due + (60 * 60 * 24 * 7);
                };
            }
            ///***************          QUARTERLY         ****************************//
            else if (itr->stake_period == QUARTERLY)
            {
                // calc payout
                payout = asset{static_cast<int64_t>((QUARTER_MULTIPLIERX100 * itr->staked.amount / 100) / total_shares * total_payout.amount / 10000), string_to_symbol(4, "EXNA")};
                if (itr->stake_date + (60 * 60 * 24 * 7 * 4 * 3) <= now()) { //if the stake_date + 3 months has expired...payout this weeks funds + add_escrow advance both dates

                  s.staked += payout;                                           // increases existing stake
                  s.staked += s.escrow;                                         // increases the stake by escrow amount
                  s.escrow -= s.escrow;                                         // zero the escrow
                  sub_balance(_self, payout + s.escrow);                        // decrement payout from _self
                  //s.stake_due = s.stake_due + (60 * 60 * 24 * 7);             // advance the stake_due by 1 week
                  //s.stake_date = s.stake_date + (60 * 60 * 24 * 7 * 4 * 3);   // advance the stake_date by 1 quartrer
                  s.stake_due = s.stake_due + (60 * 10 );                       // TESTING ONLY - Uncomment above to go-live
                  s.stake_date = s.stake_date + (60 * 20 );                     // TESTING ONLY - Uncomment above to go-live
                  add_quarterly += payout;                                      // config table book keeping
                  add_quarterly += s.escrow;                                    // config table book keeping
                  rem_escrow += s.escrow;                                       // config table book keeping
                }
                else if (itr->stake_due  + (60 * 60 * 24 * 7) <= now()){          //send payout to escrow and advance the stake_due
                  s.escrow += payout;
                  add_escrow += payout;
                  s.stake_due = s.stake_due + (60 * 60 * 24 * 7);
                };
            }

        });
        c_t.modify(c_itr, _self, [&](auto &c) {
        c.staked_weekly += add_weekly.amount;
        c.staked_monthly += add_monthly.amount;
        c.staked_quarterly += add_quarterly.amount;
        c.total_escrow += add_escrow.amount;
        c.total_escrow -= rem_escrow.amount;
        });
    }
}

void ednatoken::unstake(account_name _stake_account)
{ // ) const uint64_t _stake_id) {
    stake_table s_t(_self, _self);
    auto itr = s_t.find(_stake_account);
    require_auth(itr->stake_account);
    add_balance(itr->stake_account, itr->staked, itr->stake_account);

    config_table c_t(_self, _self);
    auto c_itr = c_t.find(0);
    c_t.modify(c_itr, _self, [&](auto &c) {

    c.active_accounts -= 1;
      if (itr->stake_period == WEEKLY) {
         c.staked_weekly -= itr->staked.amount;
       }
       else if ((itr->stake_period == MONTHLY)) {
         c.staked_monthly -= itr->staked.amount;
      }
      else if ((itr->stake_period == QUARTERLY)) {
        c.staked_quarterly -= itr->staked.amount;
      }
    });
    s_t.erase(itr);
}

void ednatoken::stake(account_name _stake_account,
                      uint8_t _stake_period,
                      asset _staked)
{

    require_auth(_stake_account);
    config_table c_t (_self, _self);
    auto c_itr = c_t.find(0);
    eosio_assert(c_itr->running != 0,"staking is currently disabled.");
    eosio_assert(is_account(_stake_account), "to account does not exist");
    auto sym = _staked.symbol.name();
    stats statstable(_self, sym);
    const auto &st = statstable.get(sym);

    eosio_assert(_staked.is_valid(), "invalid quantity");
    eosio_assert(_staked.amount > 0, "must transfer positive quantity");
    eosio_assert(_staked.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(_stake_period >= 1 && _stake_period <= 3, "Invalid stake period.");

    stake_table s_t(_self, _self);
    auto itr = s_t.find(_stake_account);
    eosio_assert(itr == s_t.end(), "Account already has a stake. Must unstake first.");

    sub_balance(_stake_account, _staked);
    asset setme = _staked;
    setme -= _staked;  // get a zero EDNA value to plug into the escrow row.
    s_t.emplace(_stake_account, [&](auto &s) {
        //    s.stake_id  = s_t.available_primary_key();
        s.stake_account = _stake_account;
        s.stake_period = _stake_period;
        s.staked = _staked;
        s.stake_date = now();
        s.escrow = setme;
        if(_stake_period == WEEKLY){
          //s.stake_due = now() + (60 * 60 * 24 * 7);
          s.stake_due = now() + (60 * 10 ); // TESTING ONLY - Uncomment above to go-live
        }
        else if(_stake_period == MONTHLY){
          //s.stake_due = now() + (60 * 60 * 24 * 7 * 4);
          s.stake_due = now() + (60 * 30 );// TESTING ONLY - Uncomment above to go-live
        }
        else if(_stake_period == QUARTERLY){
          //s.stake_due = now() + (60 * 60 * 24 * 7 * 12);
          s.stake_due = now() + (60 * 60);// TESTING ONLY - Uncomment above to go-live
        }
    });
    c_t.modify(c_itr, _self, [&](auto &c) {
        c.active_accounts += 1;
        if (_stake_period == WEEKLY)
        {
          c.staked_weekly += _staked.amount;
        }
        else if (_stake_period == MONTHLY)
        {
          c.staked_monthly += _staked.amount;
        }
        else if (_stake_period == QUARTERLY)
        {
          c.staked_quarterly += _staked.amount;
        }
    });
}

void ednatoken::run(uint8_t on_switch){
    require_auth (_self);
    config_table c_t (_self, _self);
    auto c_itr = c_t.find(0);
    if (c_itr == c_t.end()) {
        c_t.emplace (_self, [&](auto &c) {
            c.running = on_switch;
        });
    } else {
        c_t.modify(c_itr, _self, [&](auto &c) {
            c.running = on_switch;
        });
    }
}

void ednatoken::resetstats(){
  require_auth (_self);
  asset returntokens;
  config_table c_t (_self, _self);
  auto c_itr = c_t.find(0);
  c_t.modify(c_itr, _self, [&](auto &c) {

  returntokens = c.bonus;
  c.bonus = c.bonus * 0;
  c.staked_weekly = 0;
  c.staked_monthly = 0;
  c.staked_quarterly = 0;
  c.total_escrow = 0;
  c.active_accounts = 0;
  c.total_shares = 0;
  returntokens += c.unclaimed_tokens;
  c.unclaimed_tokens = c.unclaimed_tokens * 0;
});
  if(returntokens.amount > 0){
    transfer(_self, c_itr->overflow, returntokens, "returned reset tokens"); // Send returned tokens to the overflow account
  }
}

void ednatoken::sub_balance(account_name owner, asset value)
{
    accounts from_acnts(_self, owner);

    const auto &from = from_acnts.get(value.symbol.name(), "no balance object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    if (from.balance.amount == value.amount)
    {
        from_acnts.erase(from);
    }
    else
    {
        from_acnts.modify(from, owner, [&](auto &a) {
            a.balance -= value;
        });
    }
}

void ednatoken::add_balance(account_name owner, asset value, account_name ram_payer)
{
    accounts to_acnts(_self, owner);
    auto to = to_acnts.find(value.symbol.name());
    if (to == to_acnts.end())
    {
        to_acnts.emplace(ram_payer, [&](auto &a) {
            a.balance = value;
            // a.staked = asset {0, value.symbol};
        });
    }
    else
    {
        to_acnts.modify(to, 0, [&](auto &a) {
            a.balance += value;
        });
    }
}
