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

  describe("Transfer", function() {
    this.timeout(0);

    it("approved transfer", async () => {
      await createExchange();

      const ownerA = exchangeTestUtil.testContext.orderOwners[0];
      const ownerB = exchangeTestUtil.testContext.orderOwners[1];

      await exchangeTestUtil.deposit(
        ownerA,
        ownerA,
        "ETH",
        new BN(web3.utils.toWei("20", "ether"))
      );

      await exchangeTestUtil.transfer(
        ownerA,
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("2", "ether")),
        "ETH",
        new BN(web3.utils.toWei("0.01", "ether")),
        {
          authMethod: AuthMethod.EDDSA,
          amountToDeposit: new BN(0),
          feeToDeposit: new BN(0),
        }
      );

      await exchangeTestUtil.transfer(
        ownerA,
        ownerB,
        "ETH",
        new BN(web3.utils.toWei("2", "ether")),
        "ETH",
        new BN(web3.utils.toWei("0.02", "ether")),
        {
          authMethod: AuthMethod.EDDSA,
          useDualAuthoring: true,
          amountToDeposit: new BN(0),
          feeToDeposit: new BN(0),
        }
      );

      await exchangeTestUtil.submitTransactions(8, false);

      // Submit the block
      await exchangeTestUtil.submitPendingBlocks();
    });

  });
});
