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

  describe("Withdraw", function() {
    this.timeout(0);

    // it("force withdraw by account 0", async () => {
    //   await createExchange();

    //   const ownerA = exchangeTestUtil.testContext.orderOwners[0];
    //   const ownerC = exchangeTestUtil.testContext.orderOwners[2];

    //   let keyPair = exchangeTestUtil.getKeyPairEDDSA();
    //   await exchangeTestUtil.requestAccountUpdate(ownerC, "ETH", new BN(0), keyPair, {
    //     authMethod: AuthMethod.ECDSA,
    //     appointedAccountID: 0,
    //   });

    //   await exchangeTestUtil.deposit(
    //     ownerC,
    //     ownerC,
    //     "ETH",
    //     new BN(web3.utils.toWei("10", "ether"))
    //   );

    //   await exchangeTestUtil.requestWithdrawal(
    //     ownerC,
    //     "ETH",
    //     new BN(web3.utils.toWei("10", "ether")),
    //     "ETH",
    //     new BN(0),
    //     { authMethod: AuthMethod.FORCE }
    //   );

    //   await exchangeTestUtil.submitTransactions(8, false);

    //   // Submit the block
    //   await exchangeTestUtil.submitPendingBlocks();
    // });

    it("withdraw by account 0 use eddsa and ecdsa", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerC = exchangeTestUtil.testContext.orderOwners[2];

      let keyPair = exchangeTestUtil.getKeyPairEDDSA();
      await exchangeTestUtil.requestAccountUpdate(ownerC, "ETH", new BN(0), keyPair, {
        authMethod: AuthMethod.ECDSA,
        appointedAccountID: 0,
      });

      await exchangeTestUtil.deposit(
        ownerC,
        ownerC,
        "ETH",
        new BN(web3.utils.toWei("10", "ether"))
      );

      await exchangeTestUtil.requestWithdrawal(
        ownerC,
        "ETH",
        new BN(web3.utils.toWei("5", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.EDDSA }
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerC,
        "ETH",
        new BN(web3.utils.toWei("4", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.ECDSA }
      );

      await exchangeTestUtil.submitTransactions(8, false);

      // Submit the block
      await exchangeTestUtil.submitPendingBlocks();
    });

    it("withdraw by type 0, 1, 2, 3", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];
      const ownerC = exchangeTestUtil.testContext.orderOwners[2];

      await exchangeTestUtil.deposit(
        ownerA,
        ownerA,
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );
      await exchangeTestUtil.deposit(
        ownerA,
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerA,
        "ETH",
        new BN(web3.utils.toWei("10", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.EDDSA }
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("10", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.ECDSA }
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerA,
        "ETH",
        new BN(web3.utils.toWei("10", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.FORCE }
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("10", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.FORCE, signer: ownerA}
      );

      await exchangeTestUtil.submitTransactions(12, false);

      // Submit the block
      await exchangeTestUtil.submitPendingBlocks();
    });

    it("withdraw to others by type 0 and 1", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];

      await exchangeTestUtil.deposit(
        ownerA,
        ownerA,
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );
      await exchangeTestUtil.deposit(
        ownerA,
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerA,
        "ETH",
        new BN(web3.utils.toWei("10", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.EDDSA, to: ownerB }
      );
      await exchangeTestUtil.requestWithdrawal(
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("10", "ether")),
        "ETH",
        new BN(0),
        { authMethod: AuthMethod.ECDSA, to: ownerA }
      );
      await exchangeTestUtil.submitTransactions(8, false);

      // Submit the block
      await exchangeTestUtil.submitPendingBlocks();
    });

  });
});
