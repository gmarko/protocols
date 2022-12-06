import BN from "bn.js";
import { Bitstream } from "../bitstream";
import { Constants } from "../constants";
import { EdDSA } from "../eddsa";
import { fromFloat } from "../float";
import { BlockContext, ExchangeState } from "../types";

interface AppKeyUpdate {
  owner?: string;
  accountID?: number;
  feeTokenID?: number;
  fee?: BN;
  validUntil?: number;
  nonce?: number;
}

/**
 * Processes account update requests.
 */
export class AppKeyUpdateProcessor {
  public static process(
    state: ExchangeState,
    block: BlockContext,
    txData: Bitstream
  ) {
    const update = AppKeyUpdateProcessor.extractData(txData);

    const accountID = state.ownerToAccountId[update.owner];
    console.log("AppKeyUpdateProcessor", update.owner, accountID);

    const account = state.getAccount(accountID);
    account.owner = update.owner;
    account.nonce++;

    const balance = account.getBalance(update.feeTokenID);
    balance.balance.isub(update.fee);

    const operator = state.getAccount(block.operatorAccountID);
    const balanceO = operator.getBalance(update.feeTokenID);
    balanceO.balance.iadd(update.fee);

    return update;
  }

  public static extractData(data: Bitstream) {
    const update: AppKeyUpdate = {};
    let offset = 0;

    const updateType = data.extractUint8(offset);
    offset += 1;
    update.owner = data.extractAddress(offset);
    offset += 20;
    update.accountID = data.extractUint32(offset);
    offset += 4;
    update.feeTokenID = data.extractUint32(offset);
    offset += 4;
    update.fee = fromFloat(
      data.extractUint16(offset),
      Constants.Float16Encoding
    );
    offset += 2;
    update.nonce = data.extractUint32(offset);
    offset += 4;

    return update;
  }
}
