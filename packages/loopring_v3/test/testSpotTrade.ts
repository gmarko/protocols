import BN = require("bn.js");
import { Constants, roundToFloatValue } from "loopringV3.js";
import { expectThrow } from "./expectThrow";
import { BalanceSnapshot, ExchangeTestUtil } from "./testExchangeUtil";
import { AuthMethod, Deposit, SpotTrade, OrderInfo, BatchSpotTrade, BatchSpotTradeUser } from "./types";

contract("Exchange", (accounts: string[]) => {
  let exchangeTestUtil: ExchangeTestUtil;
  let exchange: any;
  let depositContract: any;
  let loopring: any;
  let exchangeID = 0;

  const createExchange = async (setupTestState: boolean = true) => {
    exchangeID = await exchangeTestUtil.createExchange(
      exchangeTestUtil.testContext.stateOwners[0],
      { setupTestState }
    );
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

  const depositAll = async () => {
    const depositETHOwner1 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[1],
      exchangeTestUtil.testContext.orderOwners[1],
      "ETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositWETHOwner1 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[1],
      exchangeTestUtil.testContext.orderOwners[1],
      "WETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositGTOOwner1 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[1],
      exchangeTestUtil.testContext.orderOwners[1],
      "GTO",
      new BN(web3.utils.toWei("200", "ether"))
    );

    const depositETHOwner0 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[0],
      exchangeTestUtil.testContext.orderOwners[0],
      "ETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositWETHOwner0 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[0],
      exchangeTestUtil.testContext.orderOwners[0],
      "WETH",
      new BN(web3.utils.toWei("20", "ether"))
    );

    const depositGTOOwner0 = await exchangeTestUtil.deposit(
      exchangeTestUtil.testContext.orderOwners[0],
      exchangeTestUtil.testContext.orderOwners[0],
      "GTO",
      new BN(web3.utils.toWei("200", "ether"))
    );
  };

  describe("SpotTrade", function() {
    this.timeout(0);

    it("SpotTrade normal", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];

      await depositAll()

      {
        const order: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: ownerA,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          storageID: 0,
          taker: ownerB
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringA: SpotTrade = {
          orderA: order,
          orderB: {
            tokenS: "GTO",
            tokenB: "WETH",
            amountS: new BN(web3.utils.toWei("4", "ether")),
            amountB: new BN(web3.utils.toWei("2", "ether")),
            owner: ownerB,
            feeBips: 0,
            fillAmountBorS: false,
            feeTokenID: 0,
            fee: new BN("21000000000000"),
            maxFee: new BN("21000000000000"),
            storageID: 0
          },
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };

        await exchangeTestUtil.setupRing(ringA, true, true, false, false);
        await exchangeTestUtil.sendRing(ringA);
      }

      await exchangeTestUtil.submitTransactions(12, false);

      // Submit the block
      await exchangeTestUtil.submitPendingBlocks();
    });

    it("SpotTrade appoint taker", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];

      await depositAll()

      {
        const order: OrderInfo = {
          tokenS: "WETH",
          tokenB: "GTO",
          amountS: new BN(web3.utils.toWei("1", "ether")),
          amountB: new BN(web3.utils.toWei("0.5", "ether")),
          owner: ownerA,
          feeBips: 18,
          fillAmountBorS: true,
          feeTokenID: 0,
          fee: new BN("12300000000000"),
          maxFee: new BN("12300000000000"),
          storageID: 0,
          taker: ownerB
        };

        const spread = new BN(web3.utils.toWei("1", "ether"));
        const ringA: SpotTrade = {
          orderA: order,
          orderB: {
            tokenS: "GTO",
            tokenB: "WETH",
            amountS: new BN(web3.utils.toWei("4", "ether")),
            amountB: new BN(web3.utils.toWei("2", "ether")),
            owner: ownerB,
            feeBips: 0,
            fillAmountBorS: false,
            feeTokenID: 0,
            fee: new BN("21000000000000"),
            maxFee: new BN("21000000000000"),
            storageID: 0
          },
          expected: {
            orderA: { filledFraction: 1.0, spread },
            orderB: { filledFraction: 0.5 }
          }
        };

        await exchangeTestUtil.setupRing(ringA, true, true, false, false);
        await exchangeTestUtil.sendRing(ringA);
      }

      await exchangeTestUtil.submitTransactions(12, false);

      // Submit the block
      await exchangeTestUtil.submitPendingBlocks();
    });

    // it("SpotTrade error taker", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerB = exchangeTestUtil.testContext.orderOwners[1];
    //   const ownerC = exchangeTestUtil.testContext.orderOwners[2];

    //   await depositAll()

    //   {
    //     const order: OrderInfo = {
    //       tokenS: "WETH",
    //       tokenB: "GTO",
    //       amountS: new BN(web3.utils.toWei("1", "ether")),
    //       amountB: new BN(web3.utils.toWei("0.5", "ether")),
    //       owner: ownerA,
    //       feeBips: 18,
    //       fillAmountBorS: true,
    //       feeTokenID: 0,
    //       fee: new BN("12300000000000"),
    //       maxFee: new BN("12300000000000"),
    //       storageID: 0,
    //       taker: ownerC
    //     };

    //     const spread = new BN(web3.utils.toWei("1", "ether"));
    //     const ringA: SpotTrade = {
    //       orderA: order,
    //       orderB: {
    //         tokenS: "GTO",
    //         tokenB: "WETH",
    //         amountS: new BN(web3.utils.toWei("4", "ether")),
    //         amountB: new BN(web3.utils.toWei("2", "ether")),
    //         owner: ownerB,
    //         feeBips: 0,
    //         fillAmountBorS: false,
    //         feeTokenID: 0,
    //         fee: new BN("21000000000000"),
    //         maxFee: new BN("21000000000000"),
    //         storageID: 0
    //       },
    //       expected: {
    //         orderA: { filledFraction: 1.0, spread },
    //         orderB: { filledFraction: 0.5 }
    //       }
    //     };

    //     await exchangeTestUtil.setupRing(ringA, true, true, false, false);
    //     await exchangeTestUtil.sendRing(ringA);
    //   }

    //   await expectThrow(exchangeTestUtil.submitTransactions(12, false), "invalid taker");
    // });

  });
});
