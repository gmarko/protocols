// Deploy protocol: LoopringV3

const LRCToken = artifacts.require("./test/tokens/LRC.sol");
const ProtocolFeeVault = artifacts.require("ProtocolFeeVault");
const BlockVerifier = artifacts.require("BlockVerifier");
const ExchangeV3 = artifacts.require("ExchangeV3");
const LoopringV3 = artifacts.require("LoopringV3");

module.exports = function(deployer, network, accounts) {
  if (network != "live" && network != "live-fork" && network != "goerli") {
    deployer.then(async () => {
      await deployer.deploy(LoopringV3, LRCToken.address, ProtocolFeeVault.address, BlockVerifier.address);
    });
  } else {
    const DGTokenAddress = "0x53c8395465a84955c95159814461466053dedede"; // DG token
    const ProtocolFeeVaultAddress = "0x7B0d44D5b2eF3A8B168FAfdcc321FAb0D9d5d08C"; // DG HomeDAO
    deployer.then(async () => {
      await deployer.deploy(LoopringV3, DGTokenAddress, ProtocolFeeVaultAddress, BlockVerifier.address);
    });
  }
};
