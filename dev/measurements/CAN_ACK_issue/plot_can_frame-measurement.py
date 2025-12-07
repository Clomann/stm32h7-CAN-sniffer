import pandas as pd
import plotly.express as px
import matplotlib.pyplot as plt

start = 500
end = 800

df = pd.read_csv(f'CAN_Bittiming-STM32 vs Reference_HSE ON no bypass-failing_7.csv', 
        sep=';', 
        decimal=',', 
        names=['Zeit','Kanal A','Kanal B','CAN'],
        skiprows=2)

fig = plt.figure(constrained_layout=True)
axs = fig.subplot_mosaic([['Left', 'TopRight'],['Left', 'BottomRight']],
                          gridspec_kw={'width_ratios':[2, 1]})
axs['Left'].set_title('Three consectutive CAN frames')
axs['TopRight'].set_title('Third frame (successfull)')
axs['BottomRight'].set_title('Second frame (failing)')
for el in axs:
    axs[el].set_xlabel('t in us')

tt = df['Zeit']
x_min = min(tt)
x_max = max(tt)
y_min = min(df['Kanal A'])
y_max = max(df['Kanal A'])
axs['Left'].plot(tt, df['Kanal A'])
axs['Left'].axis([x_min, x_max, y_min, y_max])

x_min = 700
x_max = 775
y_min = min(df['Kanal B'])
y_max = max(df['Kanal B'])

axs['TopRight'].plot(tt, df['Kanal B'])
axs['TopRight'].axis([x_min, x_max, y_min, y_max])

x_min = 440
x_max = x_min + 75
y_min = min(df['Kanal B'])
y_max = max(df['Kanal B'])

axs['BottomRight'].plot(tt, df['Kanal B'])
axs['BottomRight'].axis([x_min, x_max, y_min, y_max])

plt.show()

