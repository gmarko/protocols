import BN = require("bn.js");
import { Constants, roundToFloatValue } from "loopringV3.js";
import { expectThrow } from "./expectThrow";
// import { BalanceSnapshot, ExchangeTestUtil } from "./testExchangeUtil";
import { ExchangeTestUtil } from "./testExchangeUtil";
import { AuthMethod, Deposit, SpotTrade } from "./types";

contract("Exchange", (accounts: string[]) => {
  let exchangeTestUtil: ExchangeTestUtil;
  let exchange: any;
  let depositContract: any;
  let loopring: any;
  let exchangeID = 0;

  const depositChecked = async (from: string, to: string, token: string, amount: BN, depositFee: BN = new BN(0)) => {
    const ethAddress = exchangeTestUtil.getTokenAddress("ETH");
    const ethValue = token === ethAddress ? amount.add(depositFee) : depositFee;
    // Deposit
    await exchange.deposit(from, to, token, amount, "0x", {
      from: from,
      value: ethValue,
      gasPrice: 0
    });

    // Get the Deposit event
    const event = await exchangeTestUtil.assertEventEmitted(exchange, "DepositRequested");
    assert.equal(event.to, to, "owner unexpected");
    assert.equal(event.token, exchangeTestUtil.getTokenAddress(token), "token unexpected");
    assert(event.amount.eq(amount), "amount unexpected");
  };

  const submitWithdrawalBlockChecked = async (
    deposits: Deposit[],
    expectedSuccess?: boolean[],
    expectedTo?: string[],
    blockFee?: BN
  ) => {
    assert.equal(exchangeTestUtil.pendingBlocks[exchangeID].length, 1, "unexpected number of pending blocks");
    const block = exchangeTestUtil.pendingBlocks[exchangeID][0];

    // Block fee
    const feeRecipient = exchangeTestUtil.exchangeOperator;
    blockFee = new BN(0);
    let numWithdrawals = 0;
    for (const tx of block.internalBlock.transactions) {
      if (tx.txType === "Withdraw") {
        numWithdrawals++;
        if (tx.type >= 2) {
          blockFee.iadd(tx.withdrawalFee);
        }
      } else if (tx.txType === "Deposit") {
      }
    }

    if (expectedSuccess === undefined) {
      expectedSuccess = new Array(numWithdrawals).fill(true);
    }

    if (expectedTo === undefined) {
      expectedTo = new Array(deposits.length).fill(Constants.zeroAddress);
      for (const [i, deposit] of deposits.entries()) {
        expectedTo[i] = deposit.owner === Constants.zeroAddress ? await loopring.protocolFeeVault() : deposit.owner;
      }
    }

    // Submit the block
    await exchangeTestUtil.submitPendingBlocks();

    // Check events
    // WithdrawalCompleted events
    {
      const numEventsExpected = expectedSuccess.filter(x => x === true).length;
      const events = await exchangeTestUtil.assertEventsEmitted(exchange, "WithdrawalCompleted", numEventsExpected);
      let c = 0;
      for (const [i, deposit] of deposits.entries()) {
        if (expectedSuccess[i]) {
          assert.equal(events[c].from, deposit.owner, "from should match");
          assert.equal(events[c].to, expectedTo[i], "to should match");
          assert.equal(events[c].token, deposit.token, "token should match");
          assert(events[c].amount.eq(deposit.amount), "amount should match");
          c++;
        }
      }
      assert.equal(events.length, c, "Unexpected num WithdrawalCompleted events");
    }
    // WithdrawalFailed events
    {
      const numEventsExpected = expectedSuccess.filter(x => x === false).length;
      const events = await exchangeTestUtil.assertEventsEmitted(exchange, "WithdrawalFailed", numEventsExpected);
      let c = 0;
      for (const [i, deposit] of deposits.entries()) {
        if (!expectedSuccess[i]) {
          assert.equal(events[c].from, deposit.owner, "from should match");
          assert.equal(events[c].to, expectedTo[i], "to should match");
          assert.equal(events[c].token, deposit.token, "token should match");
          assert(events[c].amount.eq(deposit.amount), "amount should match");
          c++;
        }
      }
      assert.equal(events.length, c, "Unexpected num WithdrawalFailed events");
    }

    // Check the BlockSubmitted event
    const event = await exchangeTestUtil.assertEventEmitted(exchange, "BlockSubmitted");
    assert.equal(event.blockIdx.toNumber(), block.blockIdx, "Unexpected block idx");
  };

  const withdrawOnceChecked = async (owner: string, token: string, expectedAmount: BN) => {
    // Check how much will be withdrawn
    const onchainAmountWithdrawableBefore = await exchange.getAmountWithdrawable(owner, token);
    assert(onchainAmountWithdrawableBefore.eq(expectedAmount), "unexpected withdrawable amount");

    await exchange.withdrawFromApprovedWithdrawals([owner], [token], {
      from: exchangeTestUtil.testContext.orderOwners[10]
    });

    // Complete amount needs to be withdrawn
    const onchainAmountWithdrawableAfter = await exchange.getAmountWithdrawable(owner, token);
    assert(onchainAmountWithdrawableAfter.eq(new BN(0)), "unexpected withdrawable amount");

    // Get the WithdrawalCompleted event
    const event = await exchangeTestUtil.assertEventEmitted(exchange, "WithdrawalCompleted");
    assert.equal(event.from, owner, "from unexpected");
    assert.equal(event.to, owner, "to unexpected");
    assert.equal(event.token, token, "token unexpected");
    assert(event.amount.eq(expectedAmount), "amount unexpected");
  };

  const withdrawChecked = async (owner: string, token: string, expectedAmount: BN) => {
    // Withdraw
    await withdrawOnceChecked(owner, token, expectedAmount);
    // Withdraw again, no tokens should be transferred
    await withdrawOnceChecked(owner, token, new BN(0));
  };

  const createExchange = async (setupTestState: boolean = true, useOwnerContract: boolean = true) => {
    exchangeID = await exchangeTestUtil.createExchange(exchangeTestUtil.testContext.stateOwners[0], {
      setupTestState,
      useOwnerContract
    });
    exchange = exchangeTestUtil.exchange;
    depositContract = exchangeTestUtil.depositContract;
  };

  before(async () => {
    exchangeTestUtil = new ExchangeTestUtil();
    await exchangeTestUtil.initialize(accounts);
    exchange = exchangeTestUtil.exchange;
    loopring = exchangeTestUtil.loopringV3;
    depositContract = exchangeTestUtil.depositContract;
    exchangeID = 1;
  });

  after(async () => {
    await exchangeTestUtil.stop();
  });

  describe("DepositByTransfer", function() {
    this.timeout(0);

    it("transfer ETH to deposit ", async () => {
      await createExchange();

      const token = "ETH";
      let owner = exchangeTestUtil.testContext.orderOwners[0];

      const deposit = await exchangeTestUtil.depositByTransfer(
        owner,
        owner,
        token,
        exchangeTestUtil.getRandomSmallAmount()
      );

      const tokenAddress = exchangeTestUtil.getTokenAddress(token);
      let balance = await exchange.getUnconfirmedBalance(tokenAddress);
      console.log("getUnconfirmedBalance:", tokenAddress, balance.toString(10));

      await exchangeTestUtil.submitTransactions(4);
      await exchangeTestUtil.submitPendingBlocks();
    });

    it("transfer ERC20 to deposit ", async () => {
      await createExchange();

      const token = exchangeTestUtil.getTokenAddress("LRC");
      let owner = exchangeTestUtil.testContext.orderOwners[0];

      const deposit = await exchangeTestUtil.depositByTransfer(
        owner,
        owner,
        token,
        exchangeTestUtil.getRandomSmallAmount()
      );

      let balance = await exchange.getUnconfirmedBalance(token);
      console.log("getUnconfirmedBalance:", token, balance.toString(10));

      await exchangeTestUtil.submitTransactions(4);
      await exchangeTestUtil.submitPendingBlocks();
    });
  });
});
