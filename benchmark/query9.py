import time
import pyarrow as pa
import pandas as pd
import numpy as np

def to_bucket(q: int):
    if 1 <= q <= 20:
        return 1
    elif 21 <= q <= 40:
        return 2
    elif 41 <= q <= 60:
        return 3
    elif 61 <= q <= 80:
        return 4
    elif 81 <= q <= 100:
        return 5
    else:
        return -1

def export_df(filename: str, df: pd.DataFrame):
    schema = pa.Schema.from_pandas(df, preserve_index=False)
    table = pa.Table.from_pandas(df, preserve_index=False)
    writer = pa.ipc.new_file(filename, schema)
    writer.write(table)
    writer.close()

def run_query9(df: pd.DataFrame) -> pd.DataFrame:
    sel = df.loc[:, ["ss_quantity", "ss_ext_tax", "ss_net_paid_inc_tax"]]
    buckets = sel["ss_quantity"].transform(lambda q: to_bucket(q))
    sel["bucket"] = buckets
    filtered_sel = sel[sel["bucket"] != -1]
    agg = filtered_sel.groupby("bucket").agg(
        count=pd.NamedAgg(column="ss_quantity", aggfunc="count"),
        ss_ext_tax_avg=pd.NamedAgg(column="ss_ext_tax", aggfunc="mean"),
        ss_net_paid_inc_tax_avg=pd.NamedAgg(column="ss_net_paid_inc_tax", aggfunc="mean"),
    )

    return agg

dtypes = {
    "ss_item_sk": np.int32,
    "ss_ticket_number": np.int64,
    "ss_sold_date_sk": np.int32,
    "ss_sold_time_sk": np.int32,
    "ss_customer_sk": np.int32,
    "ss_cdemo_sk": np.int32,
    "ss_hdemo_sk": np.int32,
    "ss_addr_sk": np.int32,
    "ss_store_sk": np.int32,
    "ss_promo_sk": np.int32,
    "ss_quantity": np.int32,
    "ss_wholesale_cost": np.float32,
    "ss_list_price": np.float32,
    "ss_sales_price": np.float32,
    "ss_ext_discount_amt": np.float32,
    "ss_ext_sales_price": np.float32,
    "ss_ext_wholesale_cost": np.float32,
    "ss_ext_list_price": np.float32,
    "ss_ext_tax": np.float32,
    "ss_coupon_amt": np.float32,
    "ss_net_paid": np.float32,
    "ss_net_paid_inc_tax": np.float32,
    "ss_net_profit": np.float32,
}

# Table store_sales
df = pd.read_csv(
    "/home/anandj/data/code/reffine/benchmark/tpcds-data/store_sales_1_10.dat",
    delimiter="|",
    engine="pyarrow",
    names=list(dtypes.keys()),
).fillna(value=0).astype(dtypes)
#export_df("store_sales.arrow", df)

start = time.time()
agg = run_query9(df)
end = time.time()
print("Time: ", end - start)

print(agg)
