from abc import ABC, abstractmethod
from status import Status

class Benchmark:
    def __init__(self, program):
        self.program = program
        self.compilation_error = None
        self.energy_data = None
        self.original_code = self.set_original_code()
        self.optimization_iteration = 0
        self.set_original_energy()

    @abstractmethod
    def set_original_code(self):
        pass

    @abstractmethod
    def get_original_code(self):
        return self.original_code
    
    @abstractmethod
    def set_optimization_iteration(self, num):
        self.optimization_iteration = num + 1
    
    @abstractmethod
    def get_optimization_iteration(self):
        return self.optimization_iteration
    
    @abstractmethod
    def set_original_energy(self):
        pass
    
    @abstractmethod
    def pre_process(self):
        """
        Pre-process the data or setup required before benchmarking.
        """
        pass

    @abstractmethod
    def post_process(self, code):
        """
        Post-process the results or clean-up tasks after benchmarking.
        """
        pass

    @abstractmethod
    def compile(self):
        """
        Compile code or perform setup operations required for the benchmark.
        """
        pass

    def get_compilation_error(self):
        return self.compilation_error

    @abstractmethod
    def run_tests(self):
        """
        Execute the main tests for benchmarking.
        """
        pass

    @abstractmethod
    def measure_energy(self, optimized_code):
        """
        Measure energy usage during benchmarking.
        """
        pass

    def get_energy_data(self):
        return self.energy_data

    def static_analysis(self, optimized_code):
        if not self.compile():
            return Status.COMPILATION_ERROR
        if not self.run_tests():
            return Status.RUNTIME_ERROR_OR_TEST_FAILED
        if not self.measure_energy(optimized_code):
            return Status.ALL_TEST_PASSED
        return Status.PERFORMANCE_IMPROVED     
