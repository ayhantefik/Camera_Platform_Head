import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import tensorflow as tf
import os

# print(f"TensorFlow version = {tf.__version__}\n")
# # Set a fixed random seed value, for reproducibility, this will allow us to get
# # the same random numbers each time the notebook is run
# SEED = 1337
# np.random.seed(SEED)
# tf.random.set_seed(SEED)
# # the list of gestures that data is available for
# GEST_DIRECTION = [
# "left",
# "right"
# ]
# SAMPLES_PER_GESTURE = 119
# NUM_GESTURES = len(GEST_DIRECTION)
# # create a one-hot encoded matrix that is used in the output
# ONE_HOT_ENCODED_GESTURES = np.eye(NUM_GESTURES)
# # --- data prep --------------------------------------------------------------
# inputs, outputs = [], []

# for gesture_index, gesture in enumerate(GEST_DIRECTION):
#     output = ONE_HOT_ENCODED_GESTURES[gesture_index]
#     df = pd.read_csv(f"{gesture}.csv")

#     num_recordings = df.shape[0] // SAMPLES_PER_GESTURE
#     for i in range(num_recordings):
#         tensor = []
#         for j in range(SAMPLES_PER_GESTURE):
#             idx = i * SAMPLES_PER_GESTURE + j
#             tensor += [
#                 (df['aX'][idx] + 4)     / 8,
#                 (df['aY'][idx] + 4)     / 8,
#                 (df['aZ'][idx] + 4)     / 8,
#                 (df['gX'][idx] + 2000)  / 4000,
#                 (df['gY'][idx] + 2000)  / 4000,
#                 (df['gZ'][idx] + 2000)  / 4000
#             ]
#         inputs.append(tensor)
#         outputs.append(output)      # <-- NOW inside the loop!

# inputs  = np.array(inputs)
# outputs = np.array(outputs)

# # --- shuffle & split --------------------------------------------------------
# idx = np.random.permutation(len(inputs))
# inputs, outputs = inputs[idx], outputs[idx]

# train_end = int(0.6 * len(inputs))
# test_end  = int(0.8 * len(inputs))

# x_train, x_test, x_val = np.split(inputs,  [train_end, test_end])
# y_train, y_test, y_val = np.split(outputs, [train_end, test_end])

# # --- model ------------------------------------------------------------------
# model = tf.keras.Sequential([
#     tf.keras.layers.InputLayer(input_shape=(SAMPLES_PER_GESTURE * 6,)),
#     tf.keras.layers.Dense(50, activation='relu'),
#     tf.keras.layers.Dense(15, activation='relu'),
#     tf.keras.layers.Dense(NUM_GESTURES, activation='softmax')
# ])

# model.compile(optimizer='adam',
#               loss='categorical_crossentropy',
#               metrics=['accuracy'])

# history = model.fit(
#     x_train, y_train,
#     epochs=200,              # monitor for early stop
#     batch_size=8,
#     validation_data=(x_val, y_val)
# )

# # --- export -----------------------------------------------------------------
# converter = tf.lite.TFLiteConverter.from_keras_model(model)
# # Uncomment to quantize:
# # converter.optimizations = [tf.lite.Optimize.DEFAULT]
# tflite_model = converter.convert()

# with open("gesture_model.tflite", "wb") as f:
#     f.write(tflite_model)

# print("Model size (bytes):", os.path.getsize("gesture_model.tflite"))

def convert_tflite_to_header(input_file, output_file):
    with open(input_file, "rb") as f:
        data = f.read()

    with open(output_file, "w") as f:
        f.write("const unsigned char gesture_model_tflite[] = {\n")
        for i in range(0, len(data), 12):
            line = ", ".join(f"0x{byte:02x}" for byte in data[i:i+12])
            f.write("  " + line + ",\n")
        f.write("};\n")
        f.write(f"const unsigned int gesture_model_tflite_len = {len(data)};\n")

convert_tflite_to_header("gesture_model.tflite", "model.h")
