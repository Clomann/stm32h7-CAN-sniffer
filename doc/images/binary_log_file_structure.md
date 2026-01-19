### Ideal block sequence

```mermaid
block-beta
    columns 1
    EPOCH1_SEQ1_BLOCK1["epoch 1,  block 1"]
    EPOCH1_SEQ1_BLOCK2["epoch 1,  block 2"]
    EPOCH1_SEQ1_BLOCKddd["..."]
    EPOCH1_SEQ1_BLOCKn["epoch 1,  block n"]

    EPOCH2_SEQ1_BLOCK1["epoch 2,  block 1"]
    EPOCH2_SEQ1_BLOCK2["epoch 2,  block 2"]
    EPOCH2_SEQ1_BLOCKddd["..."]
    EPOCH2_SEQ1_BLOCKn["epoch 2,  block n"]
```

### Block interlieving between runs

It may happen, that a file cannot be opened and the logger skips to the next writable file. In this case block sequences of different epochs may interlieve.

```mermaid
block-beta
    columns 1
    EPOCH2_SEQ1_BLOCK1["epoch 2,  block 1"]
    EPOCH1_SEQ1_BLOCK2["epoch 1,  block 36"]
    EPOCH2_SEQ1_BLOCK2["epoch 2,  block 2"]
    EPOCH2_SEQ1_BLOCKddd["..."]
    EPOCH2_SEQ1_BLOCKn["epoch 2,  block n"]
```