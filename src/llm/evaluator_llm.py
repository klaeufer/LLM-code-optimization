from utils import Logger
import sys

logger = Logger("logs", sys.argv[2]).logger

def evaluator_llm(energy_data, llm_assistant):

    #extract original
    original_source_code = energy_data["original"]["source_code"]
    original_avg_energy = energy_data["original"]["avg_energy"]
    original_avg_runtime = energy_data["original"]["avg_runtime"]

    lowest_soruce_code = energy_data["lowest_avg_energy"]["source_code"]
    lowest_avg_energy = energy_data["lowest_avg_energy"]["avg_energy"]
    lowest_avg_runtime = energy_data["lowest_avg_energy"]["avg_runtime"]

    current_source_code = energy_data["current"]["source_code"]  
    current_avg_energy = energy_data["current"]["avg_energy"]
    current_avg_runtime = energy_data["current"]["avg_runtime"]

    prompt = f"""
    Based on the provided instruction, evaluate the following current code snippet in terms of time complexity, space complexity, energy usage, and performance, considering both the original and optimized code. Please provide a comprehensive analysis of the code's efficiency, energy consumption, and suggest further optimizations.

    Here is the original code snippet:
    ```
    {original_source_code}
    ```
    Average energy usage: {original_avg_energy}
    Average run time: {original_avg_runtime}

    Here is the best code snippets(the lowest energy usage):
    ```
    {lowest_soruce_code}
    ```
    Average energy usage: {lowest_avg_energy}
    Average run time: {lowest_avg_runtime}

    Here is the current code snippiets that you are tasked to optimize:
    ```
    {current_source_code}
    ```
    Average energy usage: {current_avg_energy}
    Average run time: {current_avg_runtime}

    Please respond in natural language (English) with actionable suggestions for improving the current code's performance in terms of energy usage. Provide only the best code with the lowest energy usage.
    """

    thread_id = llm_assistant.create_thread()
    evaluator_feedback = llm_assistant.create_run(user_input=prompt, thread_id = thread_id)

    return evaluator_feedback